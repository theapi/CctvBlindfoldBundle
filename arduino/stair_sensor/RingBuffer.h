/*
 *  RingBuffer.h
 *
 *
 */

// header defining the interface of the source.
#ifndef _RINGBUFFER_H
#define _RINGBUFFER_H

// include Arduino basic header.
#include <Arduino.h>

// the definition of the stack class.
template<typename T>
class RingBuffer {
  public:
    // init the stack (constructor).
    RingBuffer (byte initialSize);

    // clear the stack (destructor).
    ~RingBuffer ();

    // push an item to the stack.
    void push (const T i);
    
    // get an indexed item from the stack.
    T peek (int i) const;
    
    // the current item
    T current () const;
    
    // the previous item
    T previous (int i) const;

    // get current index
    int currentIndex () const;

    // get the number of items in the stack.
    int count () const;

    // make all the values zero.
    void zero();

    // set the printer of the stack.
    void setPrinter (Print & p);

  private:
    // exit report method in case of error.
    void exit (const char * m) const;

    Print * printer; // the printer of the stack.
    T * contents;    // the array of the stack.
    int size;        // the size of the stack.
    int top;         // the top index of the stack.
};

// init the stack (constructor).
template<typename T>
RingBuffer<T>::RingBuffer (byte initialSize) {
  size = 0;       // set the size of stack to zero.
  top = 0;        // set the initial top index of the stack.
  printer = NULL; // set the printer of stack to point nowhere.

  // allocate enough memory for the array.
  contents = (T *) malloc (sizeof (T) * initialSize);

  // if there is a memory allocation error.
  if (contents == NULL)
    exit ("STACK: insufficient memory to initialize stack.");

  // set the size of the buffer.
  size = initialSize;
  
  // fill the buffer with nulls
  for (uint8_t i = 0; i < size; i++) {
    contents[i] = NULL;
  }
}

template<typename T>
void RingBuffer<T>::zero () {
  // fill the buffer with zeros
  for (uint8_t i = 0; i < size; i++) {
    contents[i] = 0;
  }
  top = 0;
}

// clear the stack (destructor).
template<typename T>
RingBuffer<T>::~RingBuffer () {
  free (contents); // deallocate the array of the stack.

  contents = NULL; // set stack's array pointer to nowhere.
  printer = NULL;  // set the printer of stack to point nowhere.
  size = 0;        // set the size of stack to zero.
  top = 0;         // set the initial top index of the stack.
}

// push an item to the stack.
template<typename T>
void RingBuffer<T>::push (const T i) {

  top++;
  if (top >= size) {
    top = 0; 
  }
  
  // store the item to the array.
  contents[top] = i;
}

// pop an item from the stack.
template<typename T>
T RingBuffer<T>::current () const {

  // fetch the top item from the array.
  return contents[top];
}

// get the previous item i number back
template<typename T>
T RingBuffer<T>::previous (int i) const {

  int bi = top;
  // previous reading
  for (uint8_t x = 0; x <= i; x++) {
    bi--;
    if (bi < 0) bi = size -1;
  }
  
  return contents[bi];
}

// get an indexed item from the stack.
template<typename T>
T RingBuffer<T>::peek (int i) const {

  // get the top item from the array.
  return contents[i];
}

// get the number of items (fixed size as given in the constructor).
template<typename T>
int RingBuffer<T>::currentIndex () const {
  return top;
}

// get the number of items (fixed size as given in the constructor).
template<typename T>
int RingBuffer<T>::count () const {
  return size;
}

// set the printer of the stack.
template<typename T>
void RingBuffer<T>::setPrinter (Print & p) {
  printer = &p;
}

// exit report method in case of error.
template<typename T>
void RingBuffer<T>::exit (const char * m) const {
  // print the message if there is a printer.
  if (printer)
    printer->println (m);

}


#endif // _RINGBUFFER_H
