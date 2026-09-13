#include "kernel/input.h"

static struct input_event queue[INPUT_QUEUE_CAPACITY];
static uint32_t read_index;
static uint32_t write_index;
static uint32_t count;

bool input_push(const struct input_event *event) {
    if (event == 0 || count == INPUT_QUEUE_CAPACITY) {
        return false;
    }
    queue[write_index] = *event;
    write_index = (write_index + 1) % INPUT_QUEUE_CAPACITY;
    ++count;
    return true;
}

bool input_pop(struct input_event *event) {
    if (event == 0 || count == 0) {
        return false;
    }
    *event = queue[read_index];
    read_index = (read_index + 1) % INPUT_QUEUE_CAPACITY;
    --count;
    return true;
}

uint32_t input_pending(void) {
    return count;
}
