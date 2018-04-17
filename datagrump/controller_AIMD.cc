#include <iostream>
#include <math.h>

#include "controller.hh"
#include "timestamp.hh"

#define MULT_DEC 0.5
#define RTT_DELAY_TRIGGER 60

using namespace std;

static double cwd_d;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
{
  cwd_d = 1;
}

/* Get current window size, in datagrams */
unsigned int Controller::window_size()
{
  /* Return integer value of current window */
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

 //If congestion was detected, multiplicatively decrease, making
  //sure not to go below 1.
 
  
  if(after_timeout){
    cwd_d = floor(cwd_d) * MULT_DEC;
    if(cwd_d < 1){
      cwd_d = 1;
    }
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

  int rtt = timestamp_ack_received - send_timestamp_acked;
  
  //For AIMD

  //If received an ACK packet that doesn't signify congestion
  //(rtt less than timeout), then increase the window size as in TCP 
  if(rtt < RTT_DELAY_TRIGGER)
    cwd_d += 1/floor(cwd_d);
  
 

}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms()
{
  return RTT_DELAY_TRIGGER;
}
