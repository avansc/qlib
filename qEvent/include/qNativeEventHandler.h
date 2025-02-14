/*
 *  qEventLib
 *  qNativeEventHandler.h
 *
 *	Copyright (c) 2001, AVS
 *	All rights reserved.
 *
 *	Redistribution and use in source and binary forms, with or without
 *	modification, are permitted provided that the following conditions are met:
 *	1.	Redistributions of source code must retain the above copyright
 *		notice, this list of conditions and the following disclaimer.
 *	2.	Redistributions in binary form must reproduce the above copyright
 *		notice, this list of conditions and the following disclaimer in the
 *		documentation and/or other materials provided with the distribution.
 *	3.	All advertising materials mentioning features or use of this software
 *		must display the following acknowledgement:
 *		This product includes software developed by the AVS.
 *	4.	Neither the name of the AVS nor the
 *		names of its contributors may be used to endorse or promote products
 *		derived from this software without specific prior written permission.
 *
 *	THIS SOFTWARE IS PROVIDED BY AVS ''AS IS'' AND ANY
 *	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *	DISCLAIMED. IN NO EVENT SHALL AVS BE LIABLE FOR ANY
 *	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _qNativeEventHandler_h
#define	_qNativeEventHandler_h

#include "qEventHandler.h"

namespace qLib
{
	namespace Util
	{
		class qObject;
	}
	
	namespace Event
	{
		class qEvent;
		typedef void(*EVENT_FUNC_PTR)(const qEvent&, const qLib::Util::qObject&);
		
		class qNativeEventHandler : public qEventHandler
		{
		public:
			qNativeEventHandler();
			virtual ~qNativeEventHandler(){};
			
			virtual void ON_EVENT(const qEvent &_evt);
			virtual void ON_EVENT(const qEvent &_evt, const qLib::Util::qObject &_obj);
			
			void set_event_ptr(EVENT_FUNC_PTR _ptr);
			
		private:
			EVENT_FUNC_PTR on_event_ptr;
		};
	}
}

#endif