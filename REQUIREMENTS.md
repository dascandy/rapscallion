# Requirements

This is a rough sketch of the requirements this RPC framework tries to fulfill.

The used protocol and framework:

* MUST have the ability to match replies to their originating requests, e.g.  through unique identifiers.
* SHOULD have the ability to refer to previous requests in the parameter lists of new requests as an alternative for transferring these parameters by-value.
  - This makes it possible to use the result of outstanding requests (aka "future results") as parameters while composing new requests.
    Obviously, execution of, these requests will only be started, on the remote peer, when the requests they depend on have been completed.
    + Given two requests A and B, with B depending only on the result of A and parameters known before submission of A, this has the advantage that A and B can be simultaneously transmitted.
      This avoids adding the extra round-trip delay between multiple dependent requests.
      It can limit the amount of data returned, in reply messages, to those results that are desired for local processing.
  - This provides the opportunity for saving bandwidth on transmitting data that the remote peer already has access to
  - This is called "promise pipe lining"
* MUST return objects with interfaces like `std::future<T>` from proxy functions that perform remote calls.
* SHOULD accept these `std::future<T>`-like objects as parameters to the proxy functions.
  - In order to support pipe lining without explicit intervention of user code.
* MAY postpone transmission of requests until their evaluation or the reception of their results are explicitly requested.
  - Under these conditions the framework MAY drop requests before transmitting them when the local handle to their results gets dropped without being used.
  - Dependent requests MAY be grouped in a single socket send-call, thus maximizing the extent to which packets get filled.
    + When doing so the requests MUST still be ordered such that they only depend on requests serialized before them.
* MUST use only standard C++ 11
 - MUST NOT rely on later standards
 - MUST NOT use compiler/language extensions