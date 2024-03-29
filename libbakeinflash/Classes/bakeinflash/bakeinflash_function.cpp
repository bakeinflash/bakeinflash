// bakeinflash_function.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// ActionScript function.

#include "bakeinflash/bakeinflash_function.h"
#include "bakeinflash/bakeinflash_log.h"
#include "bakeinflash/bakeinflash_character.h"
#include "bakeinflash/bakeinflash_sprite.h"
#include "bakeinflash/bakeinflash_as_classes/as_array.h"

namespace bakeinflash
{
	// Invokes the function represented by a Function object.
	// public call(thisObject:Object, [parameter1:Object]) : Object

	void	as_s_function_call(const fn_call& fn)
	{
		assert(fn.this_ptr);
		if (fn.nargs > 0)
		{
			as_object* properties = fn.this_ptr;
			if (properties->m_this_ptr != NULL)
			{
				as_object* func = properties->m_this_ptr.get();
				if (func)
				{
					as_environment env;
					int nargs = 0;
					if (fn.nargs > 1)
					{
						nargs = 1;
						env.push(fn.arg(1));
					}

					*fn.result = call_method(func, &env, fn.arg(0).to_object(),
						nargs, env.get_top_index());
				}
			}
		}
	}

	as_s_function::as_s_function(const action_buffer* ab, int start, const array<with_stack_entry>& with_stack) :
		m_with_stack(with_stack),
		m_start_pc(start),
		m_length(0),
		m_is_function2(false),
		m_local_register_count(0),
		m_function2_flags(0)
	{
		// Flash stores a body(functions) of a class in _global,
		// action_buffer is in movie_def, therefore when we load another _root
		// action_buffer will be deleted and _global.MyClass will keep
		// the pointer to the removed object(action_buffer).
		// Therefore we have updated action_buffer* ==> action_buffer in as_s_function
		assert(ab);
		m_action_buffer = *ab;

		m_this_ptr = this;

		// any function MUST have prototype
		builtin_member("prototype", new as_object());

		builtin_member("call", as_s_function_call);
	}

	as_s_function::~as_s_function()
	{
	}

	void as_s_function::set_target(as_object* target)
	{
		m_target = target;
	}

	void	as_s_function::operator()(const fn_call& fn)
	// Dispatch.
	{
		// set 'this'
		assert(fn.this_ptr);
		as_object* this_ptr = fn.this_ptr;
		if (this_ptr->m_this_ptr != NULL && this_ptr != this_ptr->m_this_ptr.get())
		{
			this_ptr = this_ptr->m_this_ptr.get();
		}

		// Keep target alive during execution!
		if (m_target == NULL)
		{
			// hack, for class constructors saved in _global object
			m_target = this_ptr;
		}

		smart_ptr<as_object> target(m_target.get());

		// Function has been declared in moviclip ==> we should use its environment
		// At the same time 'this_ptr' may refers to another object
		// see testcase in .h file
		as_environment* env = m_target->get_environment();
		if (env == NULL)
		{
			env = fn.env;
		}
		assert(env);

		// Set up local stack frame, for parameters and locals.
	//	int	local_stack_top = env->m_local_frames.size();

		env->m_local_frames.push_back(NULL);

		if (m_is_function2 == false)
		{
			// Conventional function.

			// Push the arguments onto the local frame.
			int	args_to_pass = imin(fn.nargs, m_args.size());
			for (int i = 0; i < args_to_pass; i++)
			{
				assert(m_args[i].m_register == 0);
				env->set_local(m_args[i].m_name, fn.arg(i));
			}

			env->set_local("this", this_ptr);

			// Put 'super' in a local var.
			if (fn.this_ptr)
			{
				env->set_local("super", fn.this_ptr->get_proto());
			}
		}
		else
		{
			// function2: most args go in registers; any others get pushed.
			
			// Create local registers.
			env->add_local_registers(m_local_register_count);

			// Handle the explicit args.
			int	args_to_pass = imin(fn.nargs, m_args.size());
			for (int i = 0; i < args_to_pass; i++)
			{
				if (m_args[i].m_register == 0)
				{
					// Conventional arg passing: create a local var.
					env->set_local(m_args[i].m_name, fn.arg(i));
				}
				else
				{
					// Pass argument into a register.
					int	reg = m_args[i].m_register;
					env->set_register(reg, fn.arg(i));
				}
			}

			// Handle the implicit args.
			int	current_reg = 1;

			if (m_function2_flags & 0x01)
			{
				// preload 'this' into a register.
				IF_VERBOSE_ACTION(myprintf("-------------- preload this=%p to register %d\n",
					this_ptr, current_reg));
				env->set_register(current_reg, this_ptr);
				current_reg++;

			}

			if (m_function2_flags & 0x02)
			{
				// Don't put 'this' into a local var.
			}
			else
			{
				// Put 'this' in a local var.
				env->set_local("this", as_value(this_ptr));
			}

			// Init arguments array, if it's going to be needed.
			smart_ptr<as_array>	arg_array;
			if ((m_function2_flags & 0x04) || ! (m_function2_flags & 0x08))
			{
				arg_array = new as_array();

				as_value	index_number;
				for (int i = 0; i < fn.nargs; i++)
				{
					index_number.set_int(i);
					arg_array->set_member(index_number.to_string(), fn.arg(i));
				}
			}

			if (m_function2_flags & 0x04)
			{
				// preload 'arguments' into a register.
				env->set_register(current_reg, arg_array.get());
				current_reg++;
			}

			if (m_function2_flags & 0x08)
			{
				// Don't put 'arguments' in a local var.
			}
			else
			{
				// Put 'arguments' in a local var.
 				env->set_local("arguments", as_value(arg_array.get()));
			}

			if (m_function2_flags & 0x10)
			{
				// Put 'super' in a register.
				if (fn.this_ptr)
				{
					IF_VERBOSE_ACTION(myprintf("-------------- preload super=%p to register %d\n",	fn.this_ptr->get_proto(), current_reg));
					env->set_register(current_reg, fn.this_ptr->get_proto());
				}
				current_reg++;
			}

			if (m_function2_flags & 0x20)
			{
				// Don't put 'super' in a local var.
			}
			else
			{
				// Put 'super' in a local var.
				if (fn.this_ptr)
				{
					env->set_local("super", fn.this_ptr->get_proto());
				}
			}

			if (m_function2_flags & 0x40)
			{
				// Put '_root' in a register.
				env->set_register(current_reg, get_root()->get_root_movie());
				current_reg++;
			}

			if (m_function2_flags & 0x80)
			{
				// Put '_parent' in a register.
				array<with_stack_entry>	dummy;
				as_value	parent;
				env->get_variable("_parent", &parent, dummy);
				IF_VERBOSE_ACTION(myprintf("-------------- preload _parent=%p to register %d\n", parent.to_object(), current_reg));
				env->set_register(current_reg, parent);
				current_reg++;
			}

			if (m_function2_flags & 0x100)
			{
				// Put '_global' in a register.
				IF_VERBOSE_ACTION(myprintf("-------------- preload _global=%p to register %d\n", 
					get_global(), current_reg));
				env->set_register(current_reg, get_global());
				current_reg++;
			}
		}

		// keep stack size
		int stack_size = env->get_stack_size();

		// Execute the actions.
		m_action_buffer.execute(env, m_start_pc, m_length, fn.result, m_with_stack, m_is_function2);

		// restore stack size
		// it should not be but it happens
		if (stack_size != env->get_stack_size())
		{
//			myprintf("s_function: on entry stack size (%d) != on exit stack size (%d)\n", 
//				stack_size, env->m_stack.size());
			env->set_stack_size(stack_size);
		}

		// Clean up stack frame.
		delete env->m_local_frames.back();
		env->m_local_frames.pop_back();

		if (m_is_function2)
		{
			// Clean up the local registers.
			env->drop_local_registers(m_local_register_count);
		}
				
	}

	as_c_function::as_c_function(as_c_function_ptr func) :
		m_func(func)
	{
		// any function MUST have prototype
		builtin_member("prototype", new as_object());
	}

	void	as_c_function::operator()(const fn_call& fn)
	{
		if (m_func)
		{
			(*m_func)(fn);
		}
	}
}
