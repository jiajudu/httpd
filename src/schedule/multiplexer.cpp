#include "schedule/multiplexer.h"
Multiplexer::Multiplexer()
    : listeners(make_shared<ListenerPool>()),
      connectors(make_shared<ConnectorPool>()),
      connections(make_shared<ConnectionPool>()),
      events(make_shared<EventPool>()), timers(make_shared<TimerPool>()) {
    listeners->multiplexer = this;
    connectors->multiplexer = this;
    connections->multiplexer = this;
    events->multiplexer = this;
    timers->multiplexer = this;
}