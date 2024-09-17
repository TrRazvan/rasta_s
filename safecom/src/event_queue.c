#include "event_queue.h"
#include "assert.h"

/* Initialize the event queue */
void event_queue_init(EventQueue *queue)
{
    queue->front = 0;
    queue->rear = 0;
}

/* Check if the queue is empty */
uint32_t event_queue_is_empty(const EventQueue *queue)
{
    return queue->front == queue->rear;
}

/* Delete the queue */
void event_delete_queue(EventQueue *queue)
{
    queue->front = 0;
    queue->rear = 0;
}

/* Add an event to the queue */
uint32_t event_queue_push(EventQueue *queue, const Event event)
{
    if ((queue->rear + 1) % MAX_EVENTS == queue->front) {
        /* Queue is full */
        return -1;
    }
    
    queue->events[queue->rear] = event;
    queue->rear = (queue->rear + 1) % MAX_EVENTS;
    return 0;
}

/* Remove and return an event from the queue */
uint32_t event_queue_pop(EventQueue *queue, Event *event)
{
    if (event_queue_is_empty(queue)) {
        return -1;
    }

    *event = queue->events[queue->front];
    queue->front = (queue->front + 1) % MAX_EVENTS;
    return 0;
}

uint32_t get_current_time()
{
    return 0U;
}

bool check_expired_timers(SmType *self, TimerContext *timer, PDU_S *pdu)
{
    bool expired_time = false;
    uint32_t current_time = get_current_time();

    /* Check if the Th timer (for Heartbeat) has expired */
    if ((current_time - timer->last_heartbeat_time) >= timer->Th) {
        timer->last_heartbeat_time = current_time; /* Reset the Th timer */
        Sm_HandleEvent(self, EVENT_TH_ELAPSED, pdu);
    }

    /* Check if the Ti timer has expired (for message timeout) */
    if ((current_time - timer->last_message_time) >= timer->Ti) {
        timer->last_message_time = current_time; /* Reset the Ti timer */
        expired_time = true;
        Sm_HandleEvent(self, EVENT_TI_ELAPSED, pdu);
    }

    return expired_time;
}

/* Initialize Timer Context */
void timer_init(TimerContext *timer, const uint32_t Th, const uint32_t Ti)
{
    assert(timer != NULL);
    timer->Th = Th;
    timer->Ti = Ti;
    timer->last_heartbeat_time = get_current_time();
    timer->last_message_time = get_current_time();
}

void process_events(SmType *sm, EventQueue *queue, TimerContext *timer, PDU_S *pdu)
{
    Event event;

    /* Process all events in the queue */
    while (!event_queue_is_empty(queue)) {
        /* Check timers */
        if (check_expired_timers(sm, timer, pdu) == true) {
            event_delete_queue(queue);
        }
        /* Get the next event from the queue */
        if (event_queue_pop(queue, &event) == 0) {
            /* Handle the event using the state machine */
            Sm_HandleEvent(sm, event, pdu);
        }
    }
}

void on_event_received(EventQueue *queue, TimerContext *timer, SmType *sm, uint32_t message_time, Event event)
{
    /* Updates the timer for incoming messages */
    timer->last_message_time = message_time;

    /* Adds the specific event to the queue */
    event_queue_push(queue, event);
}

