# FileSend (Qt)

Build:

    cmake -S . -B build
    cmake --build build
    ./build/filesend

Needs Qt 6 (or Qt 5.15+) with the Widgets and Network modules.
Ports: UDP 6000 (discovery), TCP 5000 (transfer). Allow them through your firewall.
