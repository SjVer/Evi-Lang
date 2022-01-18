#ifndef ARRAY_H
#define ARRAY_H

#include <cstdlib>
#include <cstring>
 
template<class T>
class DArray
{
public:
    DArray(); // constructor  
    DArray(const DArray &a); // copy constructor  
    ~DArray(); // distructor  
    DArray& operator = (const DArray &a); // assignment operator  
 
    T& operator [] (unsigned int index); // get array item  
    T& pop(unsigned int pos);
    T& pop(); 
    void append(const T &item); // Add item to the end of array  
 
    unsigned int length(); // get used of array (elements) 
    void setSize(unsigned int newused); // set used of array (elements) 
    void clear(); // clear array 
    void remove(unsigned int pos); // remove array item  
	void* getptr(); // get void* pointer to array data 

	enum exception { MEMFAIL };

private:
    T *array; // pointer for array's memory  
    unsigned int used; // used of array (elemets) 
    unsigned int capacity; // actual used of allocated memory   
 
	const static int dyn_array_step = 128; // initial used of array memory (elements) 
	const static int dyn_array_mult = 2; // multiplier (enlarge array memory  
										 // dyn_array_mult times  ) 
};

// ======================================

template <class T>
DArray<T>::DArray() 
{ 
    capacity = dyn_array_step; // First, allocate step  
                               // for dyn_array_step items 
    used = 0;
    array = (T *)malloc(capacity*sizeof(T));
 
    if (array == NULL) 
        throw MEMFAIL;
} 
 
 
template <class T>
DArray<T>::~DArray() 
{ 
    if (array) 
    { 
        free(array); // Freeing memory  
        array = NULL;
    } 
} 
 
 
template <class T>
DArray<T>::DArray(const DArray &a) 
{ 
    array = (T *)malloc(sizeof(T)*a.capacity);
    if (array == NULL) 
        throw MEMFAIL;
 
    memcpy(array, a.array, sizeof(T)*a.capacity);
    // memcpy call -- coping memory contents  
    capacity = a.capacity;
    used = a.used;
} 
 
 
template <class T>
DArray<T>& DArray<T>::operator = (const DArray &a) 
{ 
    if (this == &a) // in case somebody tries assign array to itself  
        return *this;
 
    if (a.used == 0) // is other array is empty -- clear this array  
        clear();
 
    setSize(a.used); // set used  
 
    memcpy(array, a.array, sizeof(T)*a.used);
 
    return *this;
} 
 
template <class T>
unsigned int DArray<T>::length() 
{ 
    // simply return used 
    return used;
} 
 
 
template <class T>
void DArray<T>::setSize(unsigned int newused) 
{ 
    used = newused;
 
    if (used != 0) 
    { 
        // change array memory used  
        // if new used is larger than current  
        // or new used is less then half of the current  
        if ((used > capacity) || (used < capacity/2)) 
        { 
            capacity = used;
            array = (T *)realloc(array, sizeof(T)*used);
 
            if (array == NULL) 
                throw MEMFAIL;
        }
    }
    else
        clear();
}
 
template <class T>
void DArray<T>::remove(unsigned int pos)
{ 
    if (used == 1) // If array has only one element  
        clear(); // than we clear it, since it will be deleted  
    else
    { 
        // otherwise, shift array elements  
        for(unsigned int i=pos; i<used-1; i++) 
            array[i] = array[i+1];
 
        // decrease array used 
        used--;
    } 
} 

template <class T>
T& DArray<T>::pop(unsigned int pos) 
{ 
    if (used == 1) // If array has only one element  
    {
        T& popped = array[pos];
        clear();
        return popped;
    }
    else
    {
        T& popped = array[pos];

        // otherwise, shift array elements
        for(unsigned int i=pos; i<used-1; i++) 
            array[i] = array[i+1];
 
        // decrease array used 
        used--;
        return popped;
    } 
}

template <class T>
T& DArray<T>::pop() 
{
    if (used == 1) // If array has only one element  
    {
        T& popped = array[0];
        clear();
        return popped;
    }
    else return array[used--];
}
 
template <class T>
void DArray<T>::clear() // clear array memory  
{ 
    used = 0; 
    array = (T *)realloc(array, sizeof(T)*dyn_array_step); 
                  // set initial memory used again  
    capacity = dyn_array_step;
} 
 
template <class T>
void *DArray<T>::getptr() 
{ 
    // return void* pointer  
    return array;
} 
 
template <class T>
T& DArray<T>::operator [] (unsigned int index) 
{ 
    // return array element  
    return array[index];
} 
 
template <class T>
void DArray<T>::append(const T &item) 
{ 
    used++;
 
    if (used > capacity) 
    { 
        capacity *= dyn_array_mult;
 
        array = (T *)realloc(array, sizeof(T)*capacity);
 
        if (array == NULL) 
            throw MEMFAIL;
    } 
 
    array[used-1] = item;
}

#endif