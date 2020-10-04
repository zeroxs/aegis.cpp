#pragma once

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
				void set(std::function<void(T)> nf) {
					std::lock_guard<std::mutex> luck(m);
					f = nf;
				}
				bool has_future_task() {
					std::lock_guard<std::mutex> luck(m);
					return (promise_passed && !promise_ran) && f && !f_ran;
				}
				template<typename Q = T, std::enable_if_t<!std::is_void_v<Q>, int> = 0>
				void run_if_not_yet(Q v, const bool is_this_promisse) {
					std::lock_guard<std::mutex> luck(m);
					if (f && !f_ran) {
						f(v);
						f_ran = true;
						promise_ran |= is_this_promisse;
					}
					promise_passed |= is_this_promisse;
				}
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
				Future(std::future<T>&& f) : std::future<T>(std::move(f)) { }
				template<typename Q = T, std::enable_if_t<!std::is_void_v<Q>, int> = 0>
				Future(Future<T>&& f) noexcept : std::future<T>(std::move(f)) { next = f.next; *later_value.var = *f.later_value.var; got_value_already = f.got_value_already; }
				template<typename Q = T, std::enable_if_t<std::is_void_v<Q>, int> = 0>
				Future(Future<T>&& f) noexcept : std::future<T>(std::move(f)) { next = f.next; got_value_already = f.got_value_already; }
				~Future() { _end<T>(); }

				void operator=(std::future<T>&& f) { std::future<T>::operator=(std::move(f)); }
				void operator=(Future<T>&& f) { std::future<T>::operator=(std::move(f)); next = f.next; *later_value.var = *f.later_value.var; got_value_already = f.got_value_already; }

				template<typename Q = T, std::enable_if_t<!std::is_void_v<Q>, int> = 0>
				T get();
				template<typename Q = T, std::enable_if_t<std::is_void_v<Q>, int> = 0>
				void get();
				// wait is a get with no timeout. You can get() later.
				template<typename Q = T, std::enable_if_t<!std::is_void_v<Q>, int> = 0>
				void wait();
				template<typename Q = T, std::enable_if_t<std::is_void_v<Q>, int> = 0>
				void wait();
				// if got, true, if not and if it has to wait, false.
				bool get_ready(unsigned = 0);

				template<typename Q = void, std::enable_if_t<!std::is_void_v<Q> && !std::is_void_v<T>, int> = 0>
				Future<Q> then(std::function<Q(T)>);
				template<typename Q = void, std::enable_if_t<std::is_void_v<Q> && !std::is_void_v<T>, int> = 0>
				Future<Q> then(std::function<void(T)>);
				template<typename Q = void, std::enable_if_t<!std::is_void_v<Q>&& std::is_void_v<T>, int> = 0>
				Future<Q> then(std::function<Q(T)>);
				template<typename Q = void, std::enable_if_t<std::is_void_v<Q>&& std::is_void_v<T>, int> = 0>
				Future<Q> then(std::function<void(T)>);

				template<typename R, std::enable_if_t<!std::is_void_v<R>, int> = 0>
				void _set_next(std::function<void(T)>, std::shared_ptr<then_block<R>>);
				template<typename R, std::enable_if_t<std::is_void_v<R>, int> = 0>
				void _set_next(std::function<void(T)>, std::shared_ptr<then_block<R>>);

				std::shared_ptr<then_block<T>> _get_then();
			};




			template<typename T = void>
			class Promise : std::promise<T> {
				bool got_future = false;
				std::function<T(void)> task_to_do;
				std::shared_ptr<then_block<T>> then_if;
				bool got_future_once = false;
				bool set_already_skip = false;
			public:
				Promise() : std::promise<T>() {}
				// initializes with the function that it will run (that gives the final result)
				Promise(std::function<T(void)> nf) : std::promise<T>() {
					task_to_do = nf;
				}

				Future<T> get_future();

				// work function if there's one. It won't let it run two times.
				template<typename Q = T, std::enable_if_t<!std::is_void_v<Q>, int> = 0>
				void work();
				// work function if there's one. It won't let it run two times.
				template<typename Q = T, std::enable_if_t<std::is_void_v<Q>, int> = 0>
				void work();

				bool has_set();

				template<typename Q = T>
				typename std::enable_if_t<std::is_void_v<Q>, bool> set_value();
				template<typename Q = T>
				typename std::enable_if_t<!std::is_void_v<Q>, bool> set_value(Q);
			};


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
					if (!get_ready(TIMEOUT_MS)) throw std::exception("Blocked for too long! Variable can't be set!"); // if invalid after 20 sec, cancel yeah?
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
					if (!get_ready(TIMEOUT_MS)) throw std::exception("Blocked for too long! Variable can't be set!"); // if invalid after 20 sec, cancel yeah?
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
				return *later_value.var;
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
			Future<T> Promise<T>::get_future() {
				if (!got_future) got_future = true;
				else throw std::exception("For feature reasons, don't create multiple Futures from a single Promise");
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