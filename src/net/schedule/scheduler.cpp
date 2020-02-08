#include "net/schedule/scheduler.h"
Scheduler::Scheduler()
    : listeners(make_shared<ListenerPool>()),
      connectors(make_shared<ConnectorPool>()),
      connections(make_shared<ConnectionPool>()),
      events(make_shared<EventPool>()), timers(make_shared<TimerPool>()) {
    listeners->scheduler = this;
    connectors->scheduler = this;
    connections->scheduler = this;
    events->scheduler = this;
    timers->scheduler = this;
}