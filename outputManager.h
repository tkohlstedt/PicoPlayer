
#ifndef __NETUTIL_H__
#define __NETUTIL_H_

// Initialize the output worker
void outputInit(thread_ctrl *worker);
void outputTrigger(struct repeating_timer *timer_data);
void outputWork();

#endif
