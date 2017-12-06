# Requirements

The key words "MUST", "MUST NOT", "REQUIRED", "SHALL", "SHALL NOT", "SHOULD", "SHOULD NOT", "RECOMMENDED",  "MAY", and "OPTIONAL" in this document are to be interpreted as described in [RFC 2119][RFC2119].

* MUST have the ability to match replies to their originating requests, e.g.  through unique identifiers.
* MUST only rely on a single communication channel
  - This means the protocol MUST provide for a way to distinguish different message types and the framework MUST be able to demultiplex these.
  - For (byte) stream oriented transport protocols (e.g. TCP) this means: one socket on one port for each client.
  - For datagram oriented transport protocols (e.g. UDP) this means: one pair of source and destination ports/addresses for each client.
  - Performance enhancements MAY use additional parallel communication channels, correct function MUST NOT depend on that though.
* MUST support initiating of requests in _both_ directions.
  - This allows using requests initiated by the server to the client to be used for event notifications (i.e. pushing instead of (long) polling).
  - It is up to the application-level protocol to dictate under which circumstances a peer is allowed to initiate requests.
  - Only using server to client requests for events that are subscribed to is a valid option.
* MUST return objects with interfaces like `std::future<T>` from proxy functions that perform remote calls.

# Wishlist for future variant of Rapscallion:

The used transport protocol and framework:

* SHOULD have the ability to refer to previous requests in the parameter lists of new requests as an alternative for transferring these parameters by-value.
  - This makes it possible to use the result of outstanding requests (aka "future results") as parameters while composing new requests.
    Obviously, execution of, these requests will only be started, on the remote peer, when the requests they depend on have been completed.
    + Given two requests A and B, with B depending only on the result of A and parameters known before submission of A, this has the advantage that A and B can be simultaneously transmitted.
      This avoids adding the extra round-trip delay between multiple dependent requests.
      It can limit the amount of data returned, in reply messages, to those results that are desired for local processing.
  - This provides the opportunity for saving bandwidth on transmitting data that the remote peer already has access to
  - This is called "[promise pipe lining][promise-pipelining]"
* SHOULD accept these `std::future<T>`-like objects as parameters to the proxy functions.
  - In order to support pipe lining without explicit intervention of user code.
* MAY postpone transmission of requests until their evaluation or the reception of their results are explicitly requested.
  - Under these conditions the framework MAY drop requests before transmitting them when the local handle to their results gets dropped without being used.
  - Dependent requests MAY be grouped in a single socket send-call, thus maximizing the extent to which packets get filled.
    + When doing so the requests MUST still be ordered such that they only depend on requests serialized before them.

[RFC2119]: https://tools.ietf.org/html/rfc2119 "RFC 2119 - Key words for use in RFCs to Indicate Requirement Levels"
[promise-pipelining]: https://en.wikipedia.org/wiki/Futures_and_promises#Promise_pipelining "Promise pipelining"
