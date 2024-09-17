#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include <stdint.h>
#include "sm.h"

#define MAX_EVENTS 10  /* Maximum number of events in the queue */

/* Event Queue */
typedef struct {
    Event events[MAX_EVENTS];
    uint32_t front;
    uint32_t rear;
} EventQueue;

/* Example Timer Management */
typedef struct {
    uint32_t Th;
    uint32_t Ti;
    uint32_t last_heartbeat_time;
    uint32_t last_message_time;
} TimerContext;

/* Initialize the event queue */
void event_queue_init(EventQueue *queue);

/* Check if the queue is empty */
uint32_t event_queue_is_empty(const EventQueue *queue);

/* Delete the queue */
void event_delete_queue(EventQueue *queue);

/* Add an event to the queue */
uint32_t event_queue_push(EventQueue *queue, const Event event);

/* Remove and return an event from the queue */
uint32_t event_queue_pop(EventQueue *queue, Event *event);

bool check_expired_timers(SmType *self, TimerContext *timer, PDU_S *pdu);

/* Initialize Timer Context */
void timer_init(TimerContext *timer, const uint32_t Th, const uint32_t Ti);

void process_events(SmType *sm, EventQueue *queue, TimerContext *timer, PDU_S *pdu);

void on_event_received(EventQueue *queue, TimerContext *timer, SmType *sm, uint32_t message_time, Event event);

#endif