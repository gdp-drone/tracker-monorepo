//
// Created by Vishnu R Menon on 2019-06-10.
//

#ifndef TRACKER_MONO_SMA_HPP
#define TRACKER_MONO_SMA_HPP

#include <iostream>
#include <stddef.h>
#include <assert.h>


using namespace std;

class SMA {
public:
  explicit SMA(unsigned int period) :
      period(period), window(new double[period]), head(nullptr), tail(nullptr),
      total(0) {
    assert(period >= 1);
  }
  ~SMA() {
    delete[] window;
  }
  
  // Adds a value to the average, pushing one out if nescessary
  void add(double val) {
    // Special case: Initialization
    if (head == nullptr) {
      head = window;
      *head = val;
      tail = head;
      inc(tail);
      total = val;
      return;
    }
    
    // Were we already full?
    if (head == tail) {
      // Fix total-cache
      total -= *head;
      // Make room
      inc(head);
    }
    
    // Write the value in the next spot.
    *tail = val;
    inc(tail);
    
    // Update our total-cache
    total += val;
  }
  
  // Returns the average of the last P elements added to this SMA.
  // If no elements have been added yet, returns 0.0
  double avg() const {
    ptrdiff_t size = this->size();
    if (size == 0) {
      return 0; // No entries => 0 average
    }
    return total / (double) size; // Cast to double for floating point arithmetic
  }

private:
  unsigned int period;
  double * window; // Holds the values to calculate the average of.
  
  // Logically, head is before tail
  double * head; // Points at the oldest element we've stored.
  double * tail; // Points at the newest element we've stored.
  
  double total; // Cache the total so we don't sum everything each time.
  
  // Bumps the given pointer up by one.
  // Wraps to the start of the array if needed.
  void inc(double * & p) {
    if (++p >= window + period) {
      p = window;
    }
  }
  
  // Returns how many numbers we have stored.
  ptrdiff_t size() const {
    if (head == nullptr)
      return 0;
    if (head == tail)
      return period;
    return (period + tail - head) % period;
  }
};


#endif //TRACKER_MONO_SMA_HPP
