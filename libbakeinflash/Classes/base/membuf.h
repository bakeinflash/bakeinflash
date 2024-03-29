// membuf.h	-- Thatcher Ulrich 2005

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A simple memory buffer.  Similar to a string, but can hold null
// characters.


#ifndef MEMBUF_H
#define MEMBUF_H


#include "base/tu_config.h"
#include "base/utility.h"

class tu_string;


struct membuf
{
	 membuf();
	 membuf(const void* data, int size);
	 membuf(const membuf& buf);
	 membuf(const tu_string& str);
	 ~membuf();

	// Construct a read-only membuf that points at the given data,
	// instead of copying it.
	enum read_only_enum { READ_ONLY };
	 membuf(read_only_enum e, const void* data, int size);

	 int size() const { return m_size; }
	 const void* data() const { return m_data; }
	 void* data()
	{ 
//		assert(!m_read_only);
		return m_data; 
	}

	// Don't call these mutators on read-only membufs.
	
	 void resize(int new_size);
	 void append(const void* data, int size);
	 void append(const membuf& buf);
	 void append(Uint8 byte);
	 void append(int num);
	 void append(double num);
	 void remove(int size);

	 void dump();

	// We do not append the terminating '\0'.
	 void append(const tu_string& str);

	 Uint8&	operator[](int index);
	 const Uint8&	operator[](int index) const;
	 void	operator=(const membuf& buf);
	 bool	operator==(const membuf& buf) const;
	 bool	operator!=(const membuf& buf) const;

private:

	int m_size;
	int m_capacity;
	void* m_data;
	bool m_read_only;
};


#endif // MEMBUF_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
