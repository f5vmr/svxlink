# SVXReflector Federation

## Status

SVXReflector federation is implemented as an integrated component of each
participating SVXReflector. The current implementation uses the established
V2-compatible reflector transport for authentication and framing, with a
separate federation application protocol carried over that connection.

The implementation has been exercised between independently configured
SVXReflectors in both directions, including authentication, federation session
establishment, UDP registration, stream acceptance, OPUS audio forwarding,
normal stream termination and cleanup after an originating client disconnect.

The protocol and configuration should still be treated as development
interfaces until they have received broader interoperability testing.

## Provenance

This work is based solely on the official SvxLink/SVXReflector source by
Tobias Blomberg, SM0SVX, and the observed behaviour required by the G4NAB
reflector-family deployment.

GeuReflector is explicitly excluded as a source or technical reference. Its
source code, architecture and implementation details have not been consulted,
copied, translated or adapted.

## Purpose

Federation provides controlled, distributed, on-demand talkgroup routing
between autonomous SVXReflectors without an independent central federation
server and without client-side ReflectorLogic bridges between every server.

Each reflector remains locally administered. A talkgroup is exchanged with a
peer only when that reflector's own configuration and policy explicitly permit
it.

## Implemented principles

- Federation is integrated into every participating SVXReflector.
- There is no central federation process or mandatory hub.
- Peer relationships, credentials and policy are configured locally.
- Each talkgroup has independent stream and talker state.
- Different talkgroups may be active simultaneously.
- Only OPUS audio is federated.
- Encoded OPUS frames are forwarded without transcoding.
- Imported audio is delivered locally but is never re-exported.
- A locally originated stream is sent directly to every connected peer for
  which export policy permits that talkgroup.
- Import and export are independently denied by default.
- Failure or restart of a peer causes its connector to disconnect and retry.
- A peer disconnect removes its incoming streams and releases local listener
  state.

## Architecture

### `ReflectorFederation`

`ReflectorFederation` owns the local federation configuration, policy library,
trusted incoming sessions, incoming stream state, outgoing peer connectors and
locally originated stream lifecycle.

It coordinates stream start, audio and stop operations across every eligible
outgoing peer. It also reports incoming stream start and stop events to
`Reflector`, allowing ordinary local clients to receive the normal talker and
flush notifications.

### `FederationPeerConnection`

`FederationPeerConnection` represents one locally initiated connection to a
configured peer. It performs:

- V2-compatible challenge-response authentication;
- federation hello and capability negotiation;
- UDP path registration;
- TCP and UDP heartbeat maintenance;
- reconnect after connection failure;
- outgoing stream request, result and stop handling;
- outgoing OPUS audio sequencing;
- bounded audio buffering while stream acceptance is pending.

Only peers with `CONNECT=1` have a locally initiated connector.

### `FederationLibrary`

`FederationLibrary` loads and validates the JSON talkgroup and peer-policy
library. The library provides route classification and explicit per-peer import
and export decisions.

The library is read during reflector initialisation. Runtime hot reload,
rollback and route advertisement are not part of the current implementation.

## Connection model

An outgoing connector is directional. `CONNECT=1` means that the local
reflector initiates and maintains a connection to that peer. `CONNECT=0` means
that no local connector is created, although a correctly authenticated and
trusted incoming session from that peer may still be accepted.

Bidirectional federation normally uses reciprocal configuration:

1. Reflector A configures Reflector B with `CONNECT=1`.
2. Reflector B configures Reflector A with `CONNECT=1`.
3. A establishes an authenticated outgoing session to B.
4. B independently establishes an authenticated outgoing session to A.

There are therefore two directional sessions between a fully bidirectional
pair. Each direction has its own authentication, hello negotiation, UDP
registration, heartbeat state and reconnect behaviour.

Restarting either reflector is an ordinary operational event. The surviving
connector detects the lost session and retries until its peer is available
again.

## Federation access identities

Each reflector has one stable federation access identity configured by
`CALLSIGN`. The name is retained because authentication uses the existing V2
callsign/password mechanism, but the value may be an administrator-issued
service identity rather than an on-air amateur callsign.

A consistent family allocation is recommended:

```text
REFLECTOR-FUK    UK-WIDE
REFLECTOR-FNA    NORTH-AMERICA
REFLECTOR-FYK    YORKSHIRENET
REFLECTOR-FAU    AUSTRALIA
```

The access identity:

- must be unique within the federation family;
- remains stable when a hostname or address changes;
- is authorised through the receiving reflector's normal user/password
  configuration;
- must be accepted by the receiving reflector's configured callsign rules;
- is mapped to exactly one peer through `[FEDERATION_TRUST]`.

The stable `REFLECTOR_ID` is a separate identity. It identifies the reflector
in federation messages and should normally be a globally unique DNS-style
name.

## Authentication and trust

Federation reuses the existing V2-compatible challenge-response login:

1. The outgoing reflector connects using its configured federation access
   identity and `AUTH_KEY`.
2. The receiving reflector authenticates that identity using its normal
   access configuration.
3. The authenticated identity must appear in `[FEDERATION_TRUST]`.
4. The federation hello must agree with the configured peer name, expected
   `REFLECTOR_ID`, domain and supported protocol.
5. Only after these checks is the connection accepted as a federation peer.

An ordinary authenticated reflector client receives no federation privileges
unless its identity is explicitly trusted as a configured federation peer.

Peer credentials are local configuration. They are never stored in the JSON
policy library or sent as federation policy data.

## Transport and wire protocol

Federation control messages use the authenticated TCP connection. Federation
OPUS frames use its associated registered UDP path.

The federation application protocol has its own version and capability flags,
independent of the underlying reflector transport version. The current hello
negotiation uses federation protocol version 1.0.

### Control messages

| Type | Message | Purpose |
| ---: | --- | --- |
| 200 | `MsgFederationHello` | Present reflector identity, domain, library generation and capabilities |
| 201 | `MsgFederationHelloAck` | Accept the federation session and negotiated capabilities |
| 202 | `MsgFederationStreamStart` | Request an outgoing talkgroup stream |
| 203 | `MsgFederationStreamResult` | Accept or reject the requested stream |
| 204 | `MsgFederationStreamStop` | End an established or pending stream |

### UDP audio

`MsgUdpFederationAudio` identifies every audio frame by:

- originating reflector ID;
- talkgroup;
- stream ID;
- audio sequence number;
- encoded OPUS payload.

Federation audio is distinct from ordinary client `MsgUdpAudio`. A receiving
reflector validates the federation session, stream identity and sequence
before converting the payload into ordinary local reflector audio for its
listeners.

### Stream identity

A federation stream is identified by:

```text
origin_reflector_id
talkgroup
stream_id
```

The locally generated stream ID distinguishes successive streams on the same
talkgroup. The origin identity prevents a peer from presenting a stream as if
it came from another configured reflector.

## Configuration

Federation configuration is kept in `Federation.conf`, normally installed from
`Federation.conf.in`. Its directory is included through `GLOBAL/CFG_DIR` in
the main SVXReflector configuration.

Federation is inactive unless `[FEDERATION]` contains `ENABLE=1`.

### Local configuration

```ini
[FEDERATION]
ENABLE=1
DOMAIN=UK-WIDE
REFLECTOR_ID=uk-wide.example.org
CALLSIGN=REFLECTOR-FUK
LIBRARY=/etc/svxlink/federation.json
PEERS=NORTH-AMERICA

[FEDERATION_PEER_NORTH-AMERICA]
HOST=north-america.example.org
REFLECTOR_ID=north-america.example.org
PORT=35300
PROTOCOL=2
AUTH_KEY=replace-with-the-remote-access-key
CONNECT=1

[FEDERATION_TRUST]
REFLECTOR-FNA=NORTH-AMERICA
```

### `[FEDERATION]` values

| Value | Meaning |
| --- | --- |
| `ENABLE` | Enables federation when set to `1` |
| `DOMAIN` | Administrative federation domain for the local reflector |
| `REFLECTOR_ID` | Stable and unique local reflector identity |
| `CALLSIGN` | Local federation access identity used for outgoing authentication |
| `LIBRARY` | Path to the local JSON route and policy library |
| `PEERS` | Comma-separated configured peer names |

### Peer values

Each entry in `PEERS` requires a matching
`[FEDERATION_PEER_<name>]` section.

| Value | Meaning |
| --- | --- |
| `HOST` | Remote hostname or address |
| `REFLECTOR_ID` | Expected stable identity of the remote reflector; defaults to `HOST` when omitted |
| `PORT` | Remote reflector service port |
| `PROTOCOL` | Underlying reflector protocol used by the outgoing connector; currently `2` |
| `AUTH_KEY` | Password used with the local federation access identity on the remote reflector |
| `CONNECT` | Creates and maintains a local outgoing connector when set to `1` |

Peer names are policy identifiers and must match consistently between
`PEERS`, peer section names, `[FEDERATION_TRUST]` values and JSON
`peer_policy` keys.

### Incoming trust

`[FEDERATION_TRUST]` maps a remote authenticated access identity to its local
peer name:

```ini
[FEDERATION_TRUST]
REFLECTOR-FNA=NORTH-AMERICA
```

Each configured peer requires exactly one trusted incoming identity. The same
identity must also be authorised through the ordinary reflector access
configuration.

## Talkgroup library

The JSON library contains route metadata and explicit peer policy. It contains
no passwords or private keys.

```json
{
  "schema": 1,
  "generation": 3,
  "domain": "UK-WIDE",
  "routes": [
    {
      "type": "exact",
      "value": 235,
      "home": "UK-WIDE",
      "scope": "family",
      "description": "UK-wide calling"
    },
    {
      "type": "exact",
      "value": 9050,
      "home": "UK-WIDE",
      "scope": "family",
      "description": "Example shared gateway talkgroup"
    }
  ],
  "peer_policy": {
    "NORTH-AMERICA": {
      "import": ["235"],
      "export": ["235", "9050"]
    }
  }
}
```

### Routes

Only a configured route with `scope` set to `family` is eligible for
federation. A missing route, an unsupported route or any other scope leaves
the talkgroup local.

The `home` value records administrative ownership or routing metadata. It does
not by itself authorise a peer to originate the talkgroup. This permits a
family talkgroup to carry replies originating at another authorised
reflector.

### Peer policy

Every peer has independent `import` and `export` lists:

- `import` permits locally receiving a matching talkgroup from that peer;
- `export` permits sending a locally originated matching talkgroup to that
  peer.

Entries may be exact talkgroup numbers such as `235` or supported prefix
patterns such as `310*`. No match means deny.

Policy is evaluated locally. Authentication, trust, a family-scoped route and
the appropriate peer-policy match are all required; none of them alone grants
permission.

The library generation is announced during federation hello negotiation for
diagnostic and compatibility purposes. Libraries are not advertised or
remotely installed by the current implementation.

## Locally originated streams

Ordinary client audio follows the existing SVXReflector talker arbitration.
Only the accepted local talker can originate a federation stream.

When the first accepted OPUS frame arrives on a talkgroup:

1. `Reflector` confirms that the talkgroup is not occupied by an incoming
   federation stream.
2. `ReflectorFederation` creates one local stream with a non-zero stream ID.
3. It evaluates every locally configured outgoing connector.
4. It selects only peers whose connector is connected, whose UDP path is
   registered and whose export policy permits the talkgroup.
5. It sends an independent stream-start request to each selected peer.
6. The local stream is recorded once even when no peer is currently eligible,
   preventing repeated start attempts for every audio frame.

### Pending audio

Audio may arrive before a peer accepts its stream-start request. Each outgoing
peer stream therefore has a bounded pending queue:

- up to 25 encoded OPUS frames are retained;
- when full, the oldest frame is discarded;
- sequence numbers are assigned only when frames are transmitted;
- queued frames are sent in order immediately after acceptance;
- rejection or disconnect removes the corresponding outgoing stream state.

Once active, subsequent frames are sent immediately through the registered UDP
path.

### Local stream termination

The local stream ends when the existing reflector talker state ends. This
includes:

- an ordinary client flush;
- talker audio timeout;
- SQL timeout handling;
- originating client disconnect.

`ReflectorFederation` sends a stream-stop message to every matching outgoing
peer and removes the local stream state. Cleanup is tied to the central talker
transition, so the different termination paths share the same behaviour.

## Incoming streams

An incoming stream request is accepted only when:

- federation is enabled;
- the session belongs to a configured and trusted peer;
- the origin reflector ID matches that peer;
- the talkgroup and stream ID are non-zero;
- the source identity is present;
- the codec is `OPUS`;
- the talkgroup has a `family` route;
- local import policy permits that peer and talkgroup;
- the talkgroup does not already contain another incoming federation stream.

After acceptance, UDP frames must match the peer, origin, talkgroup and stream
identity. Sequence state is tracked independently for each talkgroup.

The receiving reflector:

1. announces the imported source to applicable V2 local clients with
   `MsgTalkerStart`;
2. broadcasts decoded federation payloads as ordinary encoded
   `MsgUdpAudio` frames to local listeners on that talkgroup;
3. never passes those imported frames into the outgoing local-stream path;
4. announces `MsgTalkerStop` when the stream ends;
5. sends `MsgUdpFlushSamples` to local listeners so buffered audio is released.

If the peer session disconnects, all incoming streams owned by that peer are
removed and the same local stop and flush lifecycle is emitted.

## Loop prevention and direct distribution

Loop prevention follows one simple boundary: only accepted ordinary local
client audio may enter the outgoing federation lifecycle.

`MsgUdpFederationAudio` is validated and delivered directly to local clients.
It is not converted into a new locally originated federation stream. Imported
audio therefore cannot be reflected back to its source or forwarded to a
third peer.

For a shared talkgroup across several reflectors, each possible origin must
have direct export permission and connectivity to every intended recipient.
A reflector does not act as a transit hub for another reflector's imported
stream.

A reply from a user on a receiving reflector is a new locally originated
stream. It receives a new origin reflector ID and stream ID and is distributed
according to that reflector's own export policy.

## Operational behaviour

### Startup

When federation is enabled, SVXReflector validates the static configuration,
trust mappings and JSON library during initialisation. It creates outgoing
connectors only for peers with `CONNECT=1` and then starts their asynchronous
connection attempts.

The startup log reports local identity, policy generation, route count, peer
configuration, trust mappings and outgoing connector count.

### Reconnection

Outgoing connectors reconnect after refusal, peer restart, transport failure
or heartbeat timeout. Authentication, hello negotiation and UDP registration
are repeated for every new session.

Runtime streams are session state. They are not restored across a disconnect;
the ordinary talker must establish a later stream if audio continues after
connectivity returns.

### Diagnostics

Normal logs identify:

- TCP connection and authentication;
- federation hello acceptance;
- UDP path registration;
- outgoing stream request, acceptance, rejection and stop;
- incoming stream acceptance and stop;
- disconnect and reconnect causes.

Credentials and authentication keys must not be written to logs.

## Tested behaviour

The development test environment has verified:

- default-denied export policy;
- disconnected peers receiving no stream request;
- V2 authentication and federation hello negotiation;
- confirmed UDP registration and heartbeats;
- outgoing stream pending and active states;
- OPUS audio queued before acceptance and flushed afterward;
- normal local stream stop;
- local stream cleanup after an originating client disconnect;
- direct UK-WIDE to NORTH-AMERICA audio;
- direct NORTH-AMERICA to UK-WIDE audio;
- local listener talker-start, OPUS audio, flush and talker-stop notifications
  for an imported stream.

## Current limitations

- Only OPUS federation streams are supported.
- Peer and policy configuration is loaded at startup.
- Runtime hot reload and rollback are not implemented.
- Route advertisements and remote policy distribution are not implemented.
- There is no federation administration API or dashboard interface.
- Imported streams are not relayed to additional peers.
- X.509 certification and the reflector protocol 3.0 certificate
  infrastructure are implemented by the participating reflectors and remain
  available. Federation peer access in the current implementation deliberately
  uses the accepted protocol 2.0 callsign/password challenge-response
  mechanism.
- Federation protocol and configuration compatibility are not yet declared
  stable.