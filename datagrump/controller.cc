#include <iostream>
#include <math.h>

#include "controller.hh"
#include "timestamp.hh"

#define MULT_DEC 0.9
#define RTT_DELAY_TRIGGER 100

using namespace std;

static unsigned int cwd;
static unsigned int recv_ack;
static double cwd_d;
static unsigned int last_ack;

static unsigned int min_rtt;
static unsigned int max_window;

static bool start;
static unsigned int num_needed_reset;
static unsigned int cur_num_reset;

static unsigned int cur_rtt;
static unsigned int num_start_rtt;

static bool congested;

static unsigned int decel_val;

static unsigned int next_min;
static unsigned int last_min_reset;
static unsigned int next_window_max;
/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
{
  cwd_d = 1;
  cwd = 1;
  recv_ack = 0;
  last_ack = 0;
  min_rtt = 10000;
  max_window = 1;
  start = true;
  num_needed_reset = 20;
  cur_num_reset=0;
  cur_rtt = 0;
  num_start_rtt = 0;
  congested = false;
  decel_val = 1;
  next_min = 1000;
  last_min_reset = 0;
  next_window_max = 0;
}

/* Get current window size, in datagrams */
unsigned int Controller::window_size()
{
  /* Default: fixed window size of 100 outstanding datagrams */
  // unsigned int the_window_size = cwd;
  unsigned int the_window_size = floor(cwd_d);
  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << the_window_size << endl;
  }
  
  return the_window_size;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp,
                                    /* in milliseconds */
				    const bool after_timeout
				    /* datagram was sent because of a timeout */ )
{
  /* Default: take no action */

  if ( debug_ ) {
    cerr << "At time " << send_timestamp
	 << " sent datagram " << sequence_number << " (timeout = " << after_timeout << ")\n";
  }


}

/* An ack was received */
void Controller::ack_received( const uint64_t sequence_number_acked,
			       /* what sequence number was acknowledged */
			       const uint64_t send_timestamp_acked,
			       /* when the acknowledged datagram was sent (sender's clock) */
			       const uint64_t recv_timestamp_acked,
			       /* when the acknowledged datagram was received (receiver's clock)*/
			       const uint64_t timestamp_ack_received )
                               /* when the ack was received (by sender) */
{
  /* Default: take no action */

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << " RTT was " << timestamp_ack_received - send_timestamp_acked
	 << endl;
  }

  unsigned int rtt = timestamp_ack_received - send_timestamp_acked;

  
  //Check if new minimum
  if(rtt < min_rtt){
    min_rtt = rtt;
  }
  //Check if congestion period is over (consistent decrease)
  if (rtt < 2*min_rtt && congested && !start){
    cur_num_reset ++;
    if(cur_num_reset > 2){
      congested = false;
      cur_num_reset = 0;
    }
  }
  //If the rtt is small enough, update the minimum
  //as an average. If new minimum, shoudl stay the same.
  //Also increase the window size.
  else if (rtt < 1.2*min_rtt){
    cwd_d += 2/floor(cwd_d);
    start = false;
    min_rtt = 0.9*min_rtt + 0.1*rtt;
  
  }
  //Bound the window size if RT is too high
  else{
    if (cwd_d > max_window)
      cwd_d = max_window;
  }
  //Go through additive decrease of window  if congested
  if(congested){
    cwd_d -= decel_val/floor(cwd_d);
    if(cwd_d < 1)
      cwd_d = 1;
  }
  //Update the max window size if close to the minimum RTT.
  if(rtt < 1.2*min_rtt){
    if(window_size() > max_window){
      max_window = window_size();
    }
    if(window_size() > next_window_max){
      next_window_max = window_size();
    }
  }
  //Looks like congestion. Half the cwd (making sure it doesn't go below 1)
  else if (rtt >= 3*min_rtt && !congested){
    congested = true;
    cwd_d -= 1/floor(cwd_d);
    cwd_d *= 0.5;
    if(cwd_d < 1)
      cwd_d = 1;
    
  }

  //Reset the minimum rtt and maximum window every 10 seconds in case
  //of any route changes
  if(timestamp_ack_received - last_min_reset > 10000){
    min_rtt = next_min;
    next_min = 10000;
    last_min_reset = timestamp_ack_received;
    max_window = next_window_max;
    next_window_max = 0;
  }
  else{
    if(rtt < next_min)
      next_min = rtt;
  }
  
  return;
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms()
{
  return min_rtt;
}
