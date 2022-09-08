/* 
 * Copyright 2010-2011 PathScale, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <typeinfo>
#include "abi_namespace.h"

namespace ABI_NAMESPACE
{
	/**
	 * Primitive type info, for intrinsic types.
	 */
	struct __fundamental_type_info : public std::type_info
	{
		virtual ~__fundamental_type_info();
	};
	/**
	 * Type info for arrays.  
	 */
	struct __array_type_info : public std::type_info
	{
		virtual ~__array_type_info();
	};
	/**
	 * Type info for functions.
	 */
	struct __function_type_info : public std::type_info
	{
		virtual ~__function_type_info();
		virtual bool __is_function_p() const { return true; }
	};
	/**
	 * Type info for enums.
	 */
	struct __enum_type_info : public std::type_info
	{
		virtual ~__enum_type_info();
	};

	/**
	 * Base class for class type info.  Used only for tentative definitions.
	 */
	struct __class_type_info : public std::type_info
	{
		virtual ~__class_type_info();
		/**
		 * Function implementing dynamic casts.
		 */
		virtual void *cast_to(void *obj, const struct __class_type_info *other) const;
		virtual bool __do_upcast(const ABI_NAMESPACE::
				__class_type_info *target,
		                       void **thrown_object) const
		{
			return this == target;
		}
	};

	/**
	 * Single-inheritance class type info.  This is used for classes containing
	 * a single non-virtual base class at offset 0.
	 */
	struct __si_class_type_info : public __class_type_info
	{
		virtual ~__si_class_type_info();
		const __class_type_info *__base_type;
		virtual bool __do_upcast(
		                const __cxxabiv1::__class_type_info *target,
		                void **thrown_object) const;
		virtual void *cast_to(void *obj, const struct __class_type_info *other) const;
	};

	/**
	 * Type info for base classes.  Classes with multiple bases store an array
	 * of these, one for each superclass.
	 */
	struct __base_class_type_info
	{
		const __class_type_info *__base_type;
		private:
			/**
			 * The high __offset_shift bits of this store the (signed) offset
			 * of the base class.  The low bits store flags from
			 * __offset_flags_masks.
			 */
			long __offset_flags;
			/**
			 * Flags used in the low bits of __offset_flags.
			 */
			enum __offset_flags_masks
			{
				/** This base class is virtual. */
				__virtual_mask = 0x1,
				/** This base class is public. */
				__public_mask = 0x2,
				/** The number of bits reserved for flags. */
				__offset_shift = 8
			};
		public:
			/**
			 * Returns the offset of the base class.
			 */
			long offset() const
			{
				return __offset_flags >> __offset_shift;
			}
			/**
			 * Returns the flags.
			 */
			long flags() const
			{
				return __offset_flags & ((1 << __offset_shift) - 1);
			}
			/**
			 * Returns whether this is a public base class.
			 */
			bool isPublic() const { return flags() & __public_mask; }
			/**
			 * Returns whether this is a virtual base class.
			 */
			bool isVirtual() const { return flags() & __virtual_mask; }
	};

	/**
	 * Type info for classes with virtual bases or multiple superclasses.
	 */
	struct __vmi_class_type_info : public __class_type_info
	{
		virtual ~__vmi_class_type_info();
		/** Flags describing this class.  Contains values from __flags_masks. */
		unsigned int __flags;
		/** The number of base classes. */
		unsigned int __base_count;
		/** 
		 * Array of base classes - this actually has __base_count elements, not
		 * 1.
		 */
		__base_class_type_info __base_info[1];

		/**
		 * Flags used in the __flags field.
		 */
		enum __flags_masks
		{
			/** The class has non-diamond repeated inheritance. */
			__non_diamond_repeat_mask = 0x1,
			/** The class is diamond shaped. */
			__diamond_shaped_mask = 0x2
		};
		virtual bool __do_upcast(
		                const __cxxabiv1::__class_type_info *target,
		                void **thrown_object) const;
		virtual void *cast_to(void *obj, const struct __class_type_info *other) const;
	};

	/**
	 * Base class used for both pointer and pointer-to-member type info.
	 */
	struct __pbase_type_info : public std::type_info
	{
		virtual ~__pbase_type_info();
		/**
		 * Flags.  Values from __masks.
		 */
		unsigned int __flags;
		/**
		 * The type info for the pointee.
		 */
		const std::type_info *__pointee;

		/**
		 * Masks used for qualifiers on the pointer.
		 */
		enum __masks
		{
			/** Pointer has const qualifier. */
			__const_mask = 0x1,
			/** Pointer has volatile qualifier. */
			__volatile_mask = 0x2,
			/** Pointer has restrict qualifier. */
			__restrict_mask = 0x4,
			/** Pointer points to an incomplete type. */
			__incomplete_mask = 0x8,
			/** Pointer is a pointer to a member of an incomplete class. */
			__incomplete_class_mask = 0x10
		};
		virtual bool __is_pointer_p() const { return true; }
		virtual bool __do_catch(const type_info *thrown_type,
		                        void **thrown_object,
		                        unsigned outer) const;
	};

	/**
	 * Pointer type info.
	 */
	struct __pointer_type_info : public __pbase_type_info
	{
		virtual ~__pointer_type_info();
	};

	/**
	 * Pointer to member type info.
	 */
	struct __pointer_to_member_type_info : public __pbase_type_info
	{
		virtual ~__pointer_to_member_type_info();
		/**
		 * Pointer to the class containing this member.
		 */
		const __class_type_info *__context;
	};

}
