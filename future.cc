#include <future>
#include <type_traits>
#include <utility>
#include <atomic>
#include <memory>
#include <iostream>

template <typename T>
class my_future;

class task_base {
public:
  task_base() {}
  virtual void run() = 0;
  virtual ~task_base() {};
};

template <typename F>
class task : public task_base {
  F f;
public:
  
  task(F &&f) : f(f) {}
  void run() override {
    f();
    delete this;
  }
};


template <typename T>
class my_promise_impl {
  std::promise<T> _pr;
  std::future<T> _future;
  std::atomic<bool> _avail;
  task_base *_task;

public:
  my_promise_impl() : _task{}, _future(_pr.get_future()), _avail(false) {}
  void sched_task(task_base *t) { _task = t; }

  bool available() {
    return _avail;
  }

  auto get() {
    return _future.get();
  }

  void set_value(T&& t) {
    _pr.set_value(std::forward<T>(t));
    if (_task) {
      _task->run();
      _task = nullptr;
    }
    _avail.store(true, std::memory_order_release);
  }

  ~my_promise_impl() {
    if (_task) {
      delete _task;
    }
  }
};


template <typename T>
class my_promise {
  std::shared_ptr<my_promise_impl<T>> _impl;
public:
  my_promise() : _impl(std::make_shared<my_promise_impl<T>>()) {}

  void sched_task(task_base *t) { _impl->sched_task(t); }
  bool available() {
    return _impl->available();
  }

  auto get_future() {
    return my_future<T>( *this );
  }

  auto get() {
    return _impl->get();
  }

  void set_value(T&& t) {
    _impl->set_value(std::forward<T>(t));
  }
};


template <typename T>
class my_future {
  my_promise<T> _pr;

public:
  my_future(my_promise<T> _pr) : _pr(_pr) {}

  auto get() {
    return _pr.get();
  }

  template<typename F>
  auto then(F&& f) -> my_future<std::invoke_result_t<F, T>> {
    using R = std::invoke_result_t<F, T>;
    using Promise = my_promise<R>;
    Promise Res_pr;

    if (_pr.available()) {
      Res_pr.set_value(f(_pr.get()));
    } else {
      _pr.sched_task(new task([Res_pr, f, pr = _pr]() mutable {
          Res_pr.set_value(f(pr.get()));
      }));
    }
    return my_future<R>(Res_pr);
  }
};



int main() {
  my_promise<int> x;
  auto f = x.get_future();
  auto y = f.then([](int x) {return x*8.02;}).then([](double y) { return y + 2;});
  x.set_value(1);
  std::cout << y.get() << std::endl; 
}


