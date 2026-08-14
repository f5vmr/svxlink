# SVXReflector Federation

## Status

Initial design proposal. No protocol or implementation is yet considered
stable.

## Provenance

This proposal is based solely on the official SvxLink/SVXReflector source by
Tobias Blomberg, SM0SVX, and the observed behaviour of an existing network
constructed using official SvxLink components.

GeuReflector is known to exist but has been explicitly excluded as a source or
technical reference for this development. Its source code, architecture and
implementation details will not be consulted, copied, translated or adapted.

The required behaviour and proposed design are being derived independently
from the existing G4NAB reflector-family deployment and the official SM0SVX
source.

## Purpose

Provide controlled, distributed, on-demand talkgroup routing between
autonomous SVXReflectors without requiring an independent central federation
server or multiple client-side ReflectorLogic bridges.

## Principles

- Each participating SVXReflector runs its own federation component.
- Each reflector remains under local administrative control.
- Talkgroups remain independent and may be active simultaneously.
- Protocol V2 is the initial transport.
- Protocol V3 federation transport is outside the initial scope.
- MCC-based talkgroup ownership provides the normal routing model.
- Exact talkgroup entries may override MCC routing.
- AllStar-associated talkgroups are ordinary shared talkgroups whose USRP
  gateway connects to one reflector only.
- Routing policy may be updated without restarting SVXReflector.
- Remote advertisements are always subject to local import/export policy.
- The failure of federation must not interrupt local reflector operation.
- Encoded audio should be forwarded without unnecessary transcoding.
- Duplicate paths and routing loops must be detected and rejected.

## Components

### ReflectorFederation

The component attached to each SVXReflector. It manages peers, routing policy,
runtime talkgroup state and federation streams.

### FederationPeer

Represents one authenticated V2 relationship with another SVXReflector.

### Talkgroup Library

Contains local ownership, MCC routes, exact overrides, family-wide
talkgroups, import/export permissions and peer advertisements.

### Administration Interface

A local control interface used by a command-line tool or Python dashboard.
The administration interface is not part of the real-time audio path.

## Initial topology

The first implementation may use a configured hub-and-spoke or tree topology.
The architecture must not require a permanently central federation service.

## Behavioural Scenarios

### Shared UK Talkgroup 235

TG 235 is treated as a UK-wide talkgroup. Without federation, TG 235 on one
SVXReflector is independent from a talkgroup with the same number on another
SVXReflector.

When TG 235 is explicitly shared:

1. A client becomes the accepted talker on TG 235 on one participating
   reflector.
2. The local federation component advertises the talker and stream to the
   authorised peer reflectors.
3. Each receiving reflector presents the stream as activity on its local
   TG 235.
4. Local clients selecting or monitoring TG 235 respond according to the
   existing SVXReflector rules.
5. The original and remote TG instances behave as one conversation for users.
6. No unconfigured talkgroup is federated merely because it has the same
   number.
7. Ending the originating stream releases the federated talker state on every
   participating reflector.

### Single USRP/AllStar Gateway

An AllStar node connects through USRP to one SvxLink ReflectorLogic client.
That client connects to one designated SVXReflector and selects the associated
talkgroup.

The talkgroup is then shared through reflector federation:

1. Audio from AllStar enters through the single USRP/SvxLink gateway.
2. The connected reflector accepts it as ordinary client audio.
3. Federation distributes the talkgroup to the other authorised reflectors.
4. A reply originating on any participating reflector is returned through the
   same federation path.
5. The reply reaches AllStar through the single gateway.
6. Additional USRP or ReflectorLogic connections to the other reflectors are
   not required.
7. Federation must prevent the stream from returning to its reflector of
   origin as a new talker.

The federation layer does not require special knowledge of AllStar. It treats
the USRP-connected ReflectorLogic as an ordinary client attached to a shared
talkgroup.

### Simultaneous Independent Talkgroups

Multiple shared talkgroups may be active simultaneously.

Activity on one federated talkgroup must not:

- block a talker on another talkgroup;
- mix audio with another talkgroup;
- change another talkgroup's membership;
- cause a global federation busy state;
- prevent unrelated local talkgroups from operating.

Talker state, stream identity, timers and routing must therefore be maintained
independently for every talkgroup.

## Configuration Model

Federation configuration is divided between static connection configuration
and a hot-reloadable talkgroup library.

### Static Configuration

Static configuration remains in `svxreflector.conf`. It contains local
identity, peer endpoints and credentials.

```ini
[FEDERATION]
ENABLE=0
DOMAIN=UK-WIDE
REFLECTOR_ID=uk-wide.example.org
CALLSIGN=MYCALL-FU
LIBRARY=/etc/svxlink/federation.json
PEERS=YORKSHIRENET,NORTH-AMERICA,AUSTRALIA-NZ

[FEDERATION_PEER_YORKSHIRENET]
HOST=yorkshirenet.example.org
REFLECTOR_ID=yorkshirenet.example.org
PORT=35300
PROTOCOL=2
AUTH_KEY=change-this-uk-yorkshire-key
CONNECT=1

[FEDERATION_PEER_NORTH-AMERICA]
HOST=north-america.example.org
REFLECTOR_ID=north-america.example.org
PORT=35300
PROTOCOL=2
AUTH_KEY=change-this-uk-na-key
CONNECT=1

[FEDERATION_PEER_AUSTRALIA-NZ]
HOST=australia-nz.example.org
REFLECTOR_ID=australia-nz.example.org
PORT=35300
PROTOCOL=2
AUTH_KEY=change-this-uk-australia-key
CONNECT=1

[FEDERATION_TRUST]
MYCALL-FY=YORKSHIRENET
MYCALL-FN=NORTH-AMERICA
MYCALL-FA=AUSTRALIA-NZ
```

Each peer section name must exactly match its entry in `PEERS`. Hyphens and
case are significant.

A peer `REFLECTOR_ID` defaults to its `HOST` when omitted. For production
federation it should be set explicitly so that the network endpoint can
change without changing the peer's stable federation identity.

The local federation `CALLSIGN` is authenticated by each remote reflector
using the existing V2 `[USERS]` and `[PASSWORDS]` mechanism. `FEDERATION_TRUST`
maps authenticated remote federation callsigns to their expected peer names.

`REFLECTOR_ID` must be stable and unique throughout the federation. It is not
a user callsign and must not change when a server address changes.

Static configuration and credentials are not advertised to peers.

### Hot-Reloadable Talkgroup Library

The talkgroup library contains no passwords or private keys. It may therefore
be managed by a local administration tool and selectively advertised to
trusted peers.

Initial JSON structure:

```json
{
  "schema": 1,
  "generation": 1,
  "domain": "UK-WIDE",
  "routes": [
    {
      "type": "exact",
      "value": 235,
      "home": "UK-WIDE",
      "scope": "family",
      "description": "UK-wide"
    },
    {
      "type": "prefix",
      "value": "234",
      "home": "YORKSHIRENET",
      "scope": "family",
      "description": "YorkshireNet MCC routes"
    },
    {
      "type": "exact",
      "value": 9050,
      "home": "UK-WIDE",
      "scope": "family",
      "service_anchor": "UK-WIDE",
      "description": "Example AllStar bridge"
    }
  ],
  "peer_policy": {
    "YORKSHIRENET": {
      "import": ["235", "234*"],
      "export": ["235", "9050"]
    },
    "NORTH-AMERICA": {
      "import": ["235", "310*", "311*"],
      "export": ["235", "9050"]
    }
  }
}
```

### Route Precedence

When more than one route matches a talkgroup, precedence is:

1. Exact talkgroup entry.
2. Most specific configured range.
3. Longest matching MCC or prefix.
4. No match: the talkgroup remains local.

Exact entries allow AllStar node numbers and other exceptions to override MCC
interpretation.

### Hot Reload

A library update must:

1. Be read into a temporary in-memory model.
2. Pass complete schema and policy validation.
3. Receive a generation number greater than the active generation.
4. Replace the active model atomically.
5. Retain the preceding valid generation for rollback.
6. Advertise only locally authorised route information to peers.

An invalid update must leave the active routing table unchanged.

### Local Authority

Peer advertisements are informational. A received route becomes effective
only when permitted by local import policy.

No peer may change another reflector's local policy, credentials or ownership
rules.
## Federated Stream Routing

Every federated talkgroup has one home reflector. MCC and exact-route rules
determine that home.

The home reflector is authoritative for talker arbitration on that talkgroup.

A remote reflector wishing to originate on the talkgroup sends a talker
request and stream towards the home reflector. Once accepted, the home
reflector distributes the stream directly to participating peers.

A stream received from its home reflector is delivered locally but is not
automatically forwarded to another peer.

Each federation stream must identify:

- talkgroup number;
- home reflector ID;
- originating reflector ID;
- unique stream ID;
- audio sequence number;
- stream state: request, start, audio, flush or stop.

A peer connection must multiplex multiple talkgroups. It must not inherit the
ordinary V2 client restriction of one selected talkgroup per connection.

The initial transport uses V2 shared-key authentication. Federation control and
audio messages extend the payload sufficiently to identify each independent
talkgroup and stream.

## Federation Identities

Each reflector module has one stable federation callsign. That identity may
connect to several different reflectors because callsign uniqueness is local
to each receiving SVXReflector.

Each reflector authorises the federation identities of the other three family
members but does not authorise its own identity as an incoming peer.

Each reflector pair uses a distinct authentication key. Authentication keys
are never included in the hot-shared talkgroup library.

Only one full-duplex connection is active for each reflector pair. Connection
ownership is configured or determined consistently so that both ends do not
create duplicate sessions.
