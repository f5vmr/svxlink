#!/usr/bin/env python3
"""Interactive planner and file generator for native SVXReflector federation.

The planning JSON deliberately contains no authentication keys.  The build
command requests them privately, or writes CHANGE_ME placeholders when asked.
"""

from __future__ import annotations

import argparse
import getpass
import json
import os
import re
import sys
from pathlib import Path
from typing import Any


PLAN_SCHEMA = 1
OWNERSHIP = ("domain", "peer", "network-wide")
DISTRIBUTION = ("local", "selected-peers", "network-wide")


def ask(prompt: str, default: str | None = None, required: bool = True) -> str:
    suffix = f" [{default}]" if default is not None else ""
    while True:
        value = input(f"{prompt}{suffix}: ").strip()
        if not value and default is not None:
            return default
        if value or not required:
            return value
        print("A value is required.")


def ask_bool(prompt: str, default: bool = False) -> bool:
    marker = "Y/n" if default else "y/N"
    while True:
        value = input(f"{prompt} [{marker}]: ").strip().lower()
        if not value:
            return default
        if value in ("y", "yes", "1", "true"):
            return True
        if value in ("n", "no", "0", "false"):
            return False
        print("Please answer yes or no.")


def ask_int(prompt: str, default: int | None = None, minimum: int = 0) -> int:
    while True:
        raw = ask(prompt, str(default) if default is not None else None)
        try:
            value = int(raw)
        except ValueError:
            print("Enter a whole number.")
            continue
        if value < minimum:
            print(f"Enter a value of at least {minimum}.")
            continue
        return value


def ask_choice(prompt: str, choices: tuple[str, ...], default: str) -> str:
    while True:
        value = ask(f"{prompt} ({'/'.join(choices)})", default).lower()
        if value in choices:
            return value
        print("Choose one of: " + ", ".join(choices))


def ask_csv(prompt: str, allowed: set[str], default: list[str] | None = None) -> list[str]:
    default_text = ",".join(default or [])
    while True:
        raw = ask(prompt, default_text, required=False)
        values = [item.strip() for item in raw.split(",") if item.strip()]
        unknown = sorted(set(values) - allowed)
        if unknown:
            print("Unknown peer name(s): " + ", ".join(unknown))
            continue
        return list(dict.fromkeys(values))


def safe_name(value: str) -> str:
    cleaned = re.sub(r"[^A-Za-z0-9_.-]+", "-", value).strip("-.")
    return cleaned or "reflector"


def write_json(path: Path, value: Any, mode: int = 0o644) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")
    os.chmod(temporary, mode)
    temporary.replace(path)


def write_text(path: Path, value: str, mode: int) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(value, encoding="utf-8")
    os.chmod(temporary, mode)
    temporary.replace(path)


def new_plan(path: Path) -> int:
    print("\nSVXReflector federation plan questionnaire")
    print("Authentication keys are not collected or stored in this plan.\n")

    domain = ask("Local federation DOMAIN", "UK-TEST")
    reflector = {
        "domain": domain,
        "reflector_id": ask("Stable REFLECTOR_ID"),
        "callsign": ask("Federation access CALLSIGN/service identity"),
        "library": ask("Installed federation library path", "/etc/svxlink/federation.json"),
        "enable": ask_bool("Generate Federation.conf with ENABLE=1", False),
    }

    peers: list[dict[str, Any]] = []
    print("\nEnter configured peers. Peer names must match federation policy names.")
    while ask_bool("Add a peer", not peers):
        name = ask("Peer name").upper()
        peers.append({
            "name": name,
            "host": ask("Remote hostname or address"),
            "reflector_id": ask("Expected remote REFLECTOR_ID"),
            "port": ask_int("Remote TCP/UDP service port", 35300, 1),
            "protocol": ask_int("SVXReflector client protocol", 2, 1),
            "connect": ask_bool("Maintain an outgoing connector", True),
            "trust_callsign": ask("Trusted incoming federation identity from this peer"),
        })

    peer_names = {peer["name"] for peer in peers}
    talkgroups: list[dict[str, Any]] = []
    print("\nEnter talkgroups. Leave the TG prompt blank when finished.")
    while True:
        raw = ask("Talkgroup number", required=False)
        if not raw:
            break
        try:
            tg = int(raw)
        except ValueError:
            print("Talkgroup must be a positive whole number.")
            continue
        if tg <= 0:
            print("Talkgroup must be greater than zero.")
            continue

        description = ask("Service or description")
        ownership = ask_choice("Ownership", OWNERSHIP, "domain")
        distribution = ask_choice("Distribution", DISTRIBUTION, "local")

        if ownership == "domain":
            home_default = domain
        elif ownership == "network-wide":
            home_default = "NETWORK-WIDE"
        else:
            home_default = next(iter(peer_names), "")

        home = ask("Administrative home domain or peer", home_default or None)
        service_anchor = ask("Service anchor (blank if none)", "", required=False)
        allstar_node = ask("Associated AllStar node (blank if none)", "", required=False)

        if distribution == "local":
            import_from: list[str] = []
            export_to: list[str] = []
        else:
            defaults = sorted(peer_names) if distribution == "network-wide" else []
            import_from = ask_csv("Permit import from peers (comma-separated)", peer_names, defaults)
            export_to = ask_csv("Permit export to peers (comma-separated)", peer_names, defaults)

        talkgroups.append({
            "tg": tg,
            "description": description,
            "ownership": ownership,
            "home": home,
            "distribution": distribution,
            "service_anchor": service_anchor,
            "allstar_node": int(allstar_node) if allstar_node.isdigit() else allstar_node,
            "import_from": import_from,
            "export_to": export_to,
        })

    plan = {
        "plan_schema": PLAN_SCHEMA,
        "library_generation": ask_int("Initial library generation", 1, 1),
        "reflector": reflector,
        "peers": peers,
        "talkgroups": talkgroups,
    }

    errors, warnings = validate_plan(plan)
    print_messages(errors, warnings)
    if errors:
        print("Plan was not written because validation failed.", file=sys.stderr)
        return 1

    write_json(path, plan)
    print(f"\nPlan written: {path}")
    return 0


def load_plan(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as exc:
        raise ValueError(f"Plan not found: {path}") from exc
    except json.JSONDecodeError as exc:
        raise ValueError(f"Invalid JSON in {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise ValueError("Plan root must be a JSON object")
    return value


def validate_plan(plan: dict[str, Any]) -> tuple[list[str], list[str]]:
    errors: list[str] = []
    warnings: list[str] = []
    if plan.get("plan_schema") != PLAN_SCHEMA:
        errors.append(f"plan_schema must be {PLAN_SCHEMA}")

    reflector = plan.get("reflector")
    if not isinstance(reflector, dict):
        return ["reflector must be an object"], warnings
    for field in ("domain", "reflector_id", "callsign", "library"):
        if not isinstance(reflector.get(field), str) or not reflector[field].strip():
            errors.append(f"reflector.{field} is required")
    if reflector.get("library") and not str(reflector["library"]).startswith("/"):
        warnings.append("reflector.library is normally an absolute path")

    peers = plan.get("peers", [])
    if not isinstance(peers, list):
        return errors + ["peers must be an array"], warnings
    peer_names: list[str] = []
    peer_ids: list[str] = []
    trust_ids: list[str] = []
    for index, peer in enumerate(peers):
        label = f"peers[{index}]"
        if not isinstance(peer, dict):
            errors.append(f"{label} must be an object")
            continue
        for field in ("name", "host", "reflector_id", "trust_callsign"):
            if not isinstance(peer.get(field), str) or not peer[field].strip():
                errors.append(f"{label}.{field} is required")
        name = str(peer.get("name", ""))
        if name:
            peer_names.append(name)
        peer_ids.append(str(peer.get("reflector_id", "")))
        trust_ids.append(str(peer.get("trust_callsign", "")))
        port = peer.get("port")
        if not isinstance(port, int) or not 1 <= port <= 65535:
            errors.append(f"{label}.port must be between 1 and 65535")
        if peer.get("protocol") != 2:
            warnings.append(f"{label}.protocol is not the currently documented value 2")

    for values, label in ((peer_names, "peer name"), (peer_ids, "peer reflector_id"), (trust_ids, "trust identity")):
        duplicates = sorted({item for item in values if item and values.count(item) > 1})
        if duplicates:
            errors.append(f"Duplicate {label}(s): {', '.join(duplicates)}")

    peer_set = set(peer_names)
    talkgroups = plan.get("talkgroups", [])
    if not isinstance(talkgroups, list):
        return errors + ["talkgroups must be an array"], warnings
    seen_tgs: set[int] = set()
    for index, route in enumerate(talkgroups):
        label = f"talkgroups[{index}]"
        if not isinstance(route, dict):
            errors.append(f"{label} must be an object")
            continue
        tg = route.get("tg")
        if not isinstance(tg, int) or tg <= 0:
            errors.append(f"{label}.tg must be a positive whole number")
        elif tg in seen_tgs:
            errors.append(f"Duplicate talkgroup: {tg}")
        else:
            seen_tgs.add(tg)
        if route.get("ownership") not in OWNERSHIP:
            errors.append(f"{label}.ownership must be one of {', '.join(OWNERSHIP)}")
        distribution = route.get("distribution")
        if distribution not in DISTRIBUTION:
            errors.append(f"{label}.distribution must be one of {', '.join(DISTRIBUTION)}")
        if not isinstance(route.get("home"), str) or not route["home"].strip():
            errors.append(f"{label}.home is required")
        for field in ("import_from", "export_to"):
            values = route.get(field, [])
            if not isinstance(values, list) or any(not isinstance(item, str) for item in values):
                errors.append(f"{label}.{field} must be an array of peer names")
                continue
            unknown = sorted(set(values) - peer_set)
            if unknown:
                errors.append(f"{label}.{field} contains unknown peers: {', '.join(unknown)}")
        if distribution == "local" and (route.get("import_from") or route.get("export_to")):
            errors.append(f"{label} is local but has peer permissions")
        if distribution != "local" and not (route.get("import_from") or route.get("export_to")):
            warnings.append(f"TG {tg} is federated but has no peer permissions")
        if route.get("ownership") == "network-wide" and not route.get("service_anchor"):
            warnings.append(f"Network-wide TG {tg} has no service anchor yet")

    generation = plan.get("library_generation")
    if not isinstance(generation, int) or generation < 1:
        errors.append("library_generation must be a positive whole number")
    return errors, warnings


def print_messages(errors: list[str], warnings: list[str]) -> None:
    for warning in warnings:
        print(f"WARNING: {warning}")
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)


def federation_library(plan: dict[str, Any]) -> dict[str, Any]:
    domain = plan["reflector"]["domain"]
    routes: list[dict[str, Any]] = []
    policies: dict[str, dict[str, list[str]]] = {
        peer["name"]: {"import": [], "export": []}
        for peer in plan["peers"]
    }

    for tg in plan["talkgroups"]:
        if tg["distribution"] == "local":
            continue
        route: dict[str, Any] = {
            "type": "exact",
            "value": tg["tg"],
            "home": tg["home"],
            "scope": "family",
            "description": tg["description"],
        }
        if tg.get("service_anchor"):
            route["service_anchor"] = tg["service_anchor"]
        routes.append(route)
        value = str(tg["tg"])
        for peer in tg.get("import_from", []):
            policies[peer]["import"].append(value)
        for peer in tg.get("export_to", []):
            policies[peer]["export"].append(value)

    for policy in policies.values():
        policy["import"] = sorted(set(policy["import"]), key=int)
        policy["export"] = sorted(set(policy["export"]), key=int)

    return {
        "schema": 1,
        "generation": plan["library_generation"],
        "domain": domain,
        "routes": sorted(routes, key=lambda route: route["value"]),
        "peer_policy": policies,
    }


def federation_conf(plan: dict[str, Any], keys: dict[str, str]) -> str:
    reflector = plan["reflector"]
    peers = plan["peers"]
    lines = [
        "###################################################################",
        "# Generated SVXReflector federation configuration",
        "# Review before installing as svxreflector.d/Federation.conf",
        "###################################################################",
        "",
        "[FEDERATION]",
        f"ENABLE={1 if reflector.get('enable') else 0}",
        f"DOMAIN={reflector['domain']}",
        f"REFLECTOR_ID={reflector['reflector_id']}",
        f"CALLSIGN={reflector['callsign']}",
        f"LIBRARY={reflector['library']}",
        "PEERS=" + ",".join(peer["name"] for peer in peers),
        "",
    ]
    for peer in peers:
        lines.extend([
            f"[FEDERATION_PEER_{peer['name']}]",
            f"HOST={peer['host']}",
            f"REFLECTOR_ID={peer['reflector_id']}",
            f"PORT={peer['port']}",
            f"PROTOCOL={peer['protocol']}",
            f"AUTH_KEY={keys[peer['name']]}",
            f"CONNECT={1 if peer.get('connect') else 0}",
            "",
        ])
    if peers:
        lines.append("[FEDERATION_TRUST]")
        for peer in peers:
            lines.append(f"{peer['trust_callsign']}={peer['name']}")
        lines.append("")
    return "\n".join(lines)


def check(path: Path) -> int:
    try:
        plan = load_plan(path)
    except ValueError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1
    errors, warnings = validate_plan(plan)
    print_messages(errors, warnings)
    if errors:
        return 1
    print(f"Plan is valid: {path}")
    print(f"Peers: {len(plan['peers'])}; talkgroups: {len(plan['talkgroups'])}")
    print(f"Federated routes emitted: {len(federation_library(plan)['routes'])}")
    return 0


def build(path: Path, output_root: Path, placeholders: bool) -> int:
    try:
        plan = load_plan(path)
    except ValueError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1
    errors, warnings = validate_plan(plan)
    print_messages(errors, warnings)
    if errors:
        return 1

    keys: dict[str, str] = {}
    for peer in plan["peers"]:
        name = peer["name"]
        if placeholders:
            keys[name] = "CHANGE_ME"
            continue
        key = getpass.getpass(f"AUTH_KEY for outgoing connection to {name} (blank writes CHANGE_ME): ")
        keys[name] = key or "CHANGE_ME"

    domain_dir = output_root / safe_name(plan["reflector"]["domain"])
    conf_path = domain_dir / "Federation.conf"
    library_path = domain_dir / "federation.json"
    write_text(conf_path, federation_conf(plan, keys), 0o600)
    write_json(library_path, federation_library(plan), 0o644)
    print(f"Generated: {conf_path}")
    print(f"Generated: {library_path}")
    if any(value == "CHANGE_ME" for value in keys.values()):
        print("WARNING: Federation.conf contains CHANGE_ME placeholders.")
    if not plan["reflector"].get("enable"):
        print("Safety state: ENABLE=0")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Plan and generate native SVXReflector federation configuration")
    sub = parser.add_subparsers(dest="command", required=True)

    new_parser = sub.add_parser("new", help="run the per-reflector questionnaire")
    new_parser.add_argument("plan", type=Path, help="planning JSON to create")

    check_parser = sub.add_parser("check", help="validate an existing planning JSON")
    check_parser.add_argument("plan", type=Path)

    build_parser = sub.add_parser("build", help="generate Federation.conf and federation.json")
    build_parser.add_argument("plan", type=Path)
    build_parser.add_argument("--output", type=Path, default=Path("generated"))
    build_parser.add_argument(
        "--placeholders", action="store_true",
        help="write CHANGE_ME instead of requesting authentication keys")

    args = parser.parse_args()
    if args.command == "new":
        return new_plan(args.plan)
    if args.command == "check":
        return check(args.plan)
    if args.command == "build":
        return build(args.plan, args.output, args.placeholders)
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
