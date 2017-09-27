# All private headers won't be installed.
set (private_headers
  NodeOptionsPrivate.hh
  NodePrivate.hh
  SubscribeOptionsPrivate.hh
)

ign_install_all_headers(EXCLUDE ${private_headers})
