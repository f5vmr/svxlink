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