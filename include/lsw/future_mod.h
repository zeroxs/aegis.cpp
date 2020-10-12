#pragma once

// easier way to use anywhere else
#define USE_STD_FULLY

#ifndef USE_STD_FULLY
#include "..\..\Handling\Abort\abort.h"
#endif

#include <future>
#include <functional>
#include <any>


#define TIMEOUT_MS 20000 // timeout if get() gets stuck for too long

namespace LSW {
	namespace v5 {
		namespace Tools {

			template<typename T>
			class then_block {
				std::function<void(T)> f{};
				bool promise_ran = false, promise_passed = false, f_ran = false;
				std::mutex m;
			public:
				/// <summary>
				/// <para>Sets the function (then function).</para>
				/// </summary>
				/// <param name="{std::function}">The then() function itself.</param>
				void set(std::function<void(T)> nf) {
					std::lock_guard<std::mutex> luck(m);
					f = nf;
				}

				/// <summary>
				/// <para>Gets if it has task to do (based on what have been done so far).</para>
				/// </summary>
				/// <returns>{bool} True if has task.</returns>
				bool has_future_task() {
					std::lock_guard<std::mutex> luck(m);
					return (promise_passed && !promise_ran) && f && !f_ran;
				}

				/// <summary>
				/// <para>Do the task as future/promise if needed.</para>
				/// </summary>
				/// <param name="{Q}">The value to be set.</param>
				/// <param name="{bool}">Is this promise?</param>
				template<typename Q = T, std::enable_if_t<!std::is_void_v<Q>, int> = 0>
				void run_if_not_yet(Q v, const bool is_this_promise) {
					std::lock_guard<std::mutex> luck(m);
					if (f && !f_ran) {
						f(v);
						f_ran = true;
						promise_ran |= is_this_promise;
					}
					promise_passed |= is_this_promise;
				}

				/// <summary>
				/// <para>Do the task as future/promise if needed. (void edition)</para>
				/// </summary>
				/// <param name="{bool}">Is this promise?</param>
				template<typename Q = T, std::enable_if_t<std::is_void_v<Q>, int> = 0>
				void run_if_not_yet(const bool is_this_promisse) {
					std::lock_guard<std::mutex> luck(m);
					if (f && !f_ran) {
						f();
						f_ran = true;
						promise_ran |= is_this_promisse;
					}
					promise_passed |= is_this_promisse;
				}
			};

			template<typename T>
			struct fake {
				T* var = nullptr;
				std::function<void(void)> del;
				template<typename Q = T, std::enable_if_t<!std::is_void_v<Q>, int> = 0>
				fake() { var = new T(); del = [&] { if (var) { delete var; var = nullptr; }}; }
				template<typename Q = T, std::enable_if_t<std::is_void_v<Q>, int> = 0>
				fake() {}

				~fake() { if (del) del(); }
			};

			/// <summary>
			/// <para>The Future holds or is about to hold a value in the future.</para>
			/// <para>You can use this to let a thread set the value assynchronously while doing other stuff.</para>
			/// </summary>
			template<typename T = void>
			class Future : std::future<T> {
				std::shared_ptr<then_block<T>> next = std::make_shared<then_block<T>>(); // internally a Promise<any> (thistype)
				fake<T> later_value;
				bool got_value_already = false;

				template<typename Q = T, std::enable_if_t<!std::is_void_v<Q>, int> = 0>
				void _end();
				template<typename Q = T, std::enable_if_t<std::is_void_v<Q>, int> = 0>
				void _end();
			public:

				/// <summary>
				/// <para>Move constructor.</para>
				/// </summary>
				/// <param name="{std::future}">The future to move.</param>
				Future(std::future<T>&& f) : std::future<T>(std::move(f)) { }

				/// <summary>
				/// <para>Move constructor.</para>
				/// </summary>
				/// <param name="{Future}">The Future to move.</param>
				template<typename Q = T, std::enable_if_t<!std::is_void_v<Q>, int> = 0>
				Future(Future<T>&& f) noexcept : std::future<T>(std::move(f)) { next = f.next; *later_value.var = *f.later_value.var; got_value_already = f.got_value_already; }

				/// <summary>
				/// <para>Move constructor.</para>
				/// </summary>
				/// <param name="{Future}">The Future to move.</param>
				template<typename Q = T, std::enable_if_t<std::is_void_v<Q>, int> = 0>
				Future(Future<T>&& f) noexcept : std::future<T>(std::move(f)) { next = f.next; got_value_already = f.got_value_already; }

				~Future() { _end<T>(); }


				/// <summary>
				/// <para>Assign copy.</para>
				/// </summary>
				/// <param name="{std::future}">The future to move.</param>
				void operator=(std::future<T>&& f) { std::future<T>::operator=(std::move(f)); }

				/// <summary>
				/// <para>Assign copy.</para>
				/// </summary>
				/// <param name="{Future}">The Future to move.</param>
				void operator=(Future<T>&& f) { std::future<T>::operator=(std::move(f)); next = f.next; *later_value.var = *f.later_value.var; got_value_already = f.got_value_already; }

				/// <summary>
				/// <para>Gets the value (if it is set) or waits (with timeout) for it to be set.</para>
				/// <para>* if you want no timeout, wait() before get().</para>
				/// </summary>
				/// <returns>{T} The value.</returns>
				template<typename Q = T, std::enable_if_t<!std::is_void_v<Q>, int> = 0>
				T get();

				/// <summary>
				/// <para>Gets if has set or waits for signal (with timeout).</para>
				/// <para>* if you want no timeout, wait() before get().</para>
				/// </summary>
				template<typename Q = T, std::enable_if_t<std::is_void_v<Q>, int> = 0>
				void get();

				/// <summary>
				/// <para>Waits for the signal to get the value.</para>
				/// </summary>
				template<typename Q = T, std::enable_if_t<!std::is_void_v<Q>, int> = 0>
				void wait();

				/// <summary>
				/// <para>Waits for the signal.</para>
				/// </summary>
				template<typename Q = T, std::enable_if_t<std::is_void_v<Q>, int> = 0>
				void wait();

				/// <summary>
				/// <para>Gets if the variable is set.</para>
				/// </summary>
				/// <param name="{unsigned}">Max time to wait for status.</param>
				/// <returns>{bool} True if available to get().</returns>
				// if got, true, if not and if it has to wait, false.
				bool get_ready(unsigned = 0);


				/// <summary>
				/// <para>Sets a function to be called after value is set.</para>
				/// <para>The type the function returns is the Future type this function returns.</para>
				/// </summary>
				/// <param name="{std::function}">A function that has T as argument and returns a type Q.</param>
				/// <returns>{Future} A Future of type Q.</returns>
				template<typename Q = T, std::enable_if_t<!std::is_void_v<Q> && !std::is_void_v<T>, int> = 0>
				Future<Q> then(std::function<Q(T)>);

				/// <summary>
				/// <para>Sets a function to be called after value is set.</para>
				/// <para>The type the function returns is the Future type this function returns.</para>
				/// </summary>
				/// <param name="{std::function}">A function that has T as argument and returns a type VOID.</param>
				/// <returns>{Future} A Future of type VOID.</returns>
				template<typename Q = T, std::enable_if_t<std::is_void_v<Q> && !std::is_void_v<T>, int> = 0>
				Future<Q> then(std::function<void(T)>);

				/// <summary>
				/// <para>Sets a function to be called after value is set.</para>
				/// <para>The type the function returns is the Future type this function returns.</para>
				/// </summary>
				/// <param name="{std::function}">A function that has NO arguments and returns a type Q.</param>
				/// <returns>{Future} A Future of type Q.</returns>
				template<typename Q = T, std::enable_if_t<!std::is_void_v<Q> && std::is_void_v<T>, int> = 0>
				Future<Q> then(std::function<Q(T)>);

				/// <summary>
				/// <para>Sets a function to be called after value is set.</para>
				/// <para>The type the function returns is the Future type this function returns.</para>
				/// </summary>
				/// <param name="{std::function}">A function that has NO arguments and returns a type VOID.</param>
				/// <returns>{Future} A Future of type VOID.</returns>
				template<typename Q = T, std::enable_if_t<std::is_void_v<Q> && std::is_void_v<T>, int> = 0>
				Future<Q> then(std::function<void(T)>);

				/// <summary>
				/// <para>Sets a function to be called after value is set.</para>
				/// <para>The type the function in this case is forced to be the VOID VOID.</para>
				/// </summary>
				/// <param name="{std::function}">A function that has NO arguments and returns a type VOID.</param>
				/// <returns>{Future} A Future of type VOID.</returns>
				template<typename Q = T, typename Void = void, std::enable_if_t<!std::is_void_v<Q> && std::is_void_v<Void>, int> = 0>
				Future<void> then(std::function<void(Void)>);


				/// <summary>
				/// <para>Internal method because of templates not being friends of others from the same class, but not same type.</para>
				/// </summary>
				/// <param name="{std::function}">The function it is linked to. (forward setting)</param>
				/// <param name="{std::shared_ptr}">Copy of back then_block.</param>
				template<typename R, std::enable_if_t<!std::is_void_v<R>, int> = 0>
				void _set_next(std::function<void(T)>, std::shared_ptr<then_block<R>>);

				/// <summary>
				/// <para>Internal method because of templates not being friends of others from the same class, but not same type.</para>
				/// </summary>
				/// <param name="{std::function}">The function it is linked to. (forward setting)</param>
				/// <param name="{std::shared_ptr}">Copy of back then_block.</param>
				template<typename R, std::enable_if_t<std::is_void_v<R>, int> = 0>
				void _set_next(std::function<void(T)>, std::shared_ptr<then_block<R>>);

				/// <summary>
				/// <para>Internal method to get internal then (because different types are not friends).</para>
				/// </summary>
				/// <returns>{std::shared_ptr} The then_block block.</returns>
				std::shared_ptr<then_block<T>> _get_then();
			};



			/// <summary>
			/// <para>Promise is the one that can generate a Future and set its value later.</para>
			/// </summary>
			template<typename T = void>
			class Promise : std::promise<T> {
				bool got_future = false;
				std::function<T(void)> task_to_do;
				std::shared_ptr<then_block<T>> then_if;
				bool got_future_once = false;
				bool set_already_skip = false;
			public:
				Promise(const Promise&) = delete;

				/// <summary>
				/// <para>Default constructor.</para>
				/// </summary>
				Promise() : std::promise<T>() {}

				/// <summary>
				/// <para>Move constructor.</para>
				/// </summary>
				/// <param name="{Promise}">Promise to move.</param>
				Promise(Promise&&);

				/// <summary>
				/// <para>Move.</para>
				/// </summary>
				/// <param name="{Promise}">Promise to move.</param>
				void operator=(Promise&&);

				/// <summary>
				/// <para>Constructor that sets up internal work function directly.</para>
				/// </summary>
				/// <param name="{std::function}">The function it will run when work() is called.</param>
				Promise(std::function<T(void)>);

				/// <summary>
				/// <para>Gets the Future associated to this Promise.</para>
				/// <para>Can only be called once per object.</para>
				/// </summary>
				/// <returns>{Future} The Future.</returns>
				Future<T> get_future();

				/// <summary>
				/// <para>Works (runs) the function set and sets as tasked.</para>
				/// <para>The value is set to the Future.</para>
				/// </summary>
				template<typename Q = T, std::enable_if_t<!std::is_void_v<Q>, int> = 0>
				void work();

				/// <summary>
				/// <para>Works (runs) the function set and sets as tasked.</para>
				/// <para>The Future will know the task has been done.</para>
				/// </summary>
				template<typename Q = T, std::enable_if_t<std::is_void_v<Q>, int> = 0>
				void work();

				/// <summary>
				/// <para>Has the value been set already?</para>
				/// </summary>
				/// <returns>{bool} True if has worked and/or the value has been set.</returns>
				bool has_set();

				/// <summary>
				/// <para>Sets as done and notifies Future.</para>
				/// </summary>
				/// <returns>{bool} True if it was set (not set before this call).</returns>
				template<typename Q = T>
				typename std::enable_if_t<std::is_void_v<Q>, bool> set_value();

				/// <summary>
				/// <para>Sets value and notifies Future.</para>
				/// </summary>
				/// <param name="{T}">The value to be set.</param>
				/// <returns>{bool} True if it was set (not set before this call).</returns>
				template<typename Q = T>
				typename std::enable_if_t<!std::is_void_v<Q>, bool> set_value(Q);
			};

			/// <summary>
			/// <para>In some weird cases you might need this, but it is not recommended, because this is not multithread.</para>
			/// <para>This sets a Future like a defined value already if you need to "cast" a value to a Future directly.</para>
			/// </summary>
			/// <param name="{T}">The value the Future will hold already.</param>
			/// <returns>{Future} The future of type T with the value you've set.</returns>
			template<typename T>
			Future<T> fake_future(T set) {
				Promise<T> pr;
				Future<T> fut = pr.get_future();
				pr.set_value(set);
				return std::move(fut);
			}



			// IMPLEMENTATION - IMPLEMENTATION - IMPLEMENTATION - IMPLEMENTATION - IMPLEMENTATION - IMPLEMENTATION - IMPLEMENTATION - IMPLEMENTATION - IMPLEMENTATION //

			/* * * * * * * * * * * * * * * * * * > FUTURE<T> < * * * * * * * * * * * * * * * * * */

			template<typename T>
			template<typename Q, std::enable_if_t<!std::is_void_v<Q>, int>>
			void Future<T>::_end() {
				if (next->has_future_task()) next->run_if_not_yet(get(), false);
			}
			template<typename T>
			template<typename Q, std::enable_if_t<std::is_void_v<Q>, int>>
			void Future<T>::_end() {
				if (next->has_future_task()) next->run_if_not_yet(false);
			}

			template<typename T>
			template<typename Q, std::enable_if_t<!std::is_void_v<Q>, int>>
			T Future<T>::get() {
				if (std::future<T>::valid()) {
#ifndef USE_STD_FULLY
					if (!get_ready(TIMEOUT_MS)) throw Handling::Abort(__FUNCSIG__, "Blocked for too long! Variable can't be set!", Handling::abort::abort_level::GIVEUP); // if invalid after 10 sec, cancel?
#else
					if (!get_ready(TIMEOUT_MS)) throw std::exception("Blocked for too long! Variable can't be set!"); // if invalid after 10 sec, cancel?
#endif
					*later_value.var = std::future<T>::get();
					if (next->has_future_task()) next->run_if_not_yet(*later_value.var, false);
					got_value_already = true;
				}
				return *later_value.var;
			}
			template<typename T>
			template<typename Q, std::enable_if_t<std::is_void_v<Q>, int>>
			void Future<T>::get() {
				if (std::future<T>::valid()) {
#ifndef USE_STD_FULLY
					if (!get_ready(TIMEOUT_MS)) throw Handling::Abort(__FUNCSIG__, "Blocked for too long! Variable can't be set!", Handling::abort::abort_level::GIVEUP); // if invalid after 10 sec, cancel?
#else
					if (!get_ready(TIMEOUT_MS)) throw std::exception("Blocked for too long! Variable can't be set!"); // if invalid after 10 sec, cancel?
#endif
					std::future<T>::get();
					if (next->has_future_task()) next->run_if_not_yet(false);
					got_value_already = true;
				}
			}

			template<typename T>
			template<typename Q, std::enable_if_t<!std::is_void_v<Q>, int>>
			void Future<T>::wait() {
				if (std::future<T>::valid()) {
					*later_value.var = std::future<T>::get(); // just wait in get
					if (next->has_future_task()) next->run_if_not_yet(*later_value.var, false);
					got_value_already = true;
				}
			}
			template<typename T>
			template<typename Q, std::enable_if_t<std::is_void_v<Q>, int>>
			void Future<T>::wait() {
				if (std::future<T>::valid()) {
					std::future<T>::get(); // just wait in get
					if (next->has_future_task()) next->run_if_not_yet(false);
					got_value_already = true;
				}
			}
			// if got, true, if not and if it has to wait, false.
			template<typename T>
			bool Future<T>::get_ready(unsigned ms) {
				return got_value_already ? true : (std::future<T>::valid() ? std::future<T>::wait_for(std::chrono::milliseconds(ms)) == std::future_status::ready : false);
			}

			template<typename T>
			template<typename Q, std::enable_if_t<!std::is_void_v<Q> && !std::is_void_v<T>, int>>
			Future<Q> Future<T>::then(std::function<Q(T)> f) {
				std::shared_ptr<Promise<Q>> nuxt = std::make_shared<Promise<Q>>();
				Future<Q> yee = nuxt->get_future();

				_set_next([ff = std::move(f), nuu = std::move(nuxt)](T res){
					nuu->set_value(ff(res));
				}, next);
				return yee;
			}

			template<typename T>
			template<typename Q, std::enable_if_t<std::is_void_v<Q> && !std::is_void_v<T>, int>>
			Future<Q> Future<T>::then(std::function<void(T)> f) {
				std::shared_ptr<Promise<void>> nuxt = std::make_shared<Promise<void>>();
				Future<void> yee = nuxt->get_future();

				_set_next([ff = std::move(f), nuu = std::move(nuxt)](T res){
					ff(res);
					nuu->set_value();
				}, next);
				return yee;
			}

			template<typename T>
			template<typename Q, std::enable_if_t<!std::is_void_v<Q>&& std::is_void_v<T>, int>>
			Future<Q> Future<T>::then(std::function<Q(T)> f) {
				std::shared_ptr<Promise<Q>> nuxt = std::make_shared<Promise<Q>>();
				Future<Q> yee = nuxt->get_future();

				_set_next([ff = std::move(f), nuu = std::move(nuxt)](){
					nuu->set_value(ff());
				}, next);
				return yee;
			}

			template<typename T>
			template<typename Q, std::enable_if_t<std::is_void_v<Q>&& std::is_void_v<T>, int>>
			Future<Q> Future<T>::then(std::function<void(T)> f) {
				std::shared_ptr<Promise<void>> nuxt = std::make_shared<Promise<void>>();
				Future<void> yee = nuxt->get_future();

				_set_next([ff = std::move(f), nuu = std::move(nuxt)](){
					ff();
					nuu->set_value();
				}, next);
				return yee;
			}

			template<typename T>
			template<typename Q, typename Void, std::enable_if_t<!std::is_void_v<Q>&& std::is_void_v<Void>, int>>
			Future<void> Future<T>::then(std::function<void(Void)> f) {
				std::shared_ptr<Promise<void>> nuxt = std::make_shared<Promise<void>>();
				Future<void> yee = nuxt->get_future();

				_set_next([ff = std::move(f), nuu = std::move(nuxt)](){
					ff();
					nuu->set_value();
				}, next);
				return yee;
			}

			template<typename T>
			template<typename R, std::enable_if_t<!std::is_void_v<R>, int>>
			void Future<T>::_set_next(std::function<void(T)> f, std::shared_ptr<then_block<R>> bfor) {
				if (bfor->has_future_task()) bfor->run_if_not_yet(*later_value.var, false);
				next->set(f);
			}

			template<typename T>
			template<typename R, std::enable_if_t<std::is_void_v<R>, int>>
			void Future<T>::_set_next(std::function<void(T)> f, std::shared_ptr<then_block<R>> bfor) {
				if (bfor->has_future_task()) bfor->run_if_not_yet(false);
				next->set(f);
			}

			template<typename T>
			std::shared_ptr<then_block<T>> Future<T>::_get_then() {
				return next;
			}

			/* * * * * * * * * * * * * * * * * * > PROMISE<T> < * * * * * * * * * * * * * * * * * */

			template<typename T>
			Promise<T>::Promise(Promise&& p) {
				got_future = std::move(p.got_future);
				task_to_do = std::move(p.task_to_do);
				then_if = std::move(p.then_if);
				got_future_once = std::move(p.got_future_once);
				set_already_skip = std::move(p.set_already_skip);
				std::promise<T>::operator=(std::move(p));
			}

			template<typename T>
			void Promise<T>::operator=(Promise&& p) {
				got_future = std::move(p.got_future);
				task_to_do = std::move(p.task_to_do);
				then_if = std::move(p.then_if);
				got_future_once = std::move(p.got_future_once);
				set_already_skip = std::move(p.set_already_skip);
				std::promise<T>::operator=(std::move(p));
			}

			template<typename T>
			Promise<T>::Promise(std::function<T(void)> nf) : std::promise<T>() {
				task_to_do = nf;
			}

			template<typename T>
			Future<T> Promise<T>::get_future() {
				if (!got_future) got_future = true;
#ifndef USE_STD_FULLY
				else throw Handling::Abort(__FUNCSIG__, "For feature reasons, don't create multiple Futures from a single Promise", Handling::abort::abort_level::GIVEUP);
#else
				else throw std::exception("For feature reasons, don't create multiple Futures from a single Promise");
#endif
				Future<T> fut = std::future<T>(std::promise<T>::get_future());
				then_if = fut._get_then();
				return fut;
			}

			template<typename T>
			template<typename Q, std::enable_if_t<!std::is_void_v<Q>, int>>
			void Promise<T>::work() {
				if (task_to_do) {
					try {
						set_value(task_to_do());
					}
					catch (std::exception e) {
						std::promise<T>::set_exception(std::current_exception());
					}
					task_to_do = std::function<T(void)>();
				}
			}

			template<typename T>
			template<typename Q, std::enable_if_t<std::is_void_v<Q>, int>>
			void Promise<T>::work() {
				if (task_to_do) {
					try {
						task_to_do();
						set_value();
					}
					catch (std::exception e) {
						std::promise<T>::set_exception(std::current_exception());
					}
					task_to_do = std::function<T(void)>();
				}
			}

			template<typename T>
			bool Promise<T>::has_set() {
				return !set_already_skip;
			}


			template<typename T>
			template<typename Q>
			typename std::enable_if_t<std::is_void_v<Q>, bool> Promise<T>::set_value() {
				if (!set_already_skip) {
					std::promise<T>::set_value();
					if (then_if) {
						then_if->run_if_not_yet(true);
						then_if.reset();
					}
					set_already_skip = true;
					return true;
				}
				return false;
			}


			template<typename T>
			template<typename Q>
			typename std::enable_if_t<!std::is_void_v<Q>, bool> Promise<T>::set_value(Q v) {
				if (!set_already_skip) {
					std::promise<T>::set_value(v);
					if (then_if) {
						then_if->run_if_not_yet(v, true);
						then_if.reset();
					}
					set_already_skip = true;
					return true;
				}
				return false;
			}
		}
	}
}