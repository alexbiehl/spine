
#ifdef LINUX
  #include "poll_epoll.c"
#else
  #ifdef MACOSX
    #include "poll_kqueue.c"
  #else
    #error "at this point i should fall back to select poll, my bad not implemented yet"
  #endif
#endif
