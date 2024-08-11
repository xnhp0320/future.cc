#include <type_traits>
#include <utility>
#include <atomic>
#include <memory>
#include <iostream>
#include <cassert>

template <typename T>
class my_future;

class task_base {
public:
  task_base() {}
  virtual void run() = 0;
  virtual ~task_base() {};
};

class future_state_base {
  bool _avail;

public:
  bool available() {
    return _avail;
  }
  void set_ready() {
    _avail = true;
  }
  future_state_base() : _avail(false) {}
};


template <typename T>
class future_state : public future_state_base {
  T _state;
public:
  future_state() : future_state_base() {} 

  T&& get() { return std::move(_state); }

  future_state(T&& v) { _state = std::forward<T>(v); }

  void set(T&& value) { _state = std::forward<T>(value); set_ready(); }
};


template <typename F, typename T>
class task : public task_base {
  F _f;
  future_state<T> _state;
public:

  task(F &&f) : _f(std::forward<F>(f)) {}
  void run() override {
    _f(&_state);
    delete this;
  }

  future_state<T>* get_state() {
    return &_state;
  }
};

template<typename T>
class future;

template <typename T>
class promise {
  future<T> *_future;
  future_state<T> *_state;
  task_base *_task;

public:
  friend class future<T>;
  promise(future<T> *f) : _state(&f->_state), _task(), _future(f) { f->_pr = this; }
  promise(): _state(), _task(), _future() {}

  promise(promise<T> &&other) {
    // if promise is moved, we should make sure the future is detached.
    _future = other._future;
    _state = other._state;
    _task = other._task;

    if (_future) {
      _future->_pr = this;
    }

    other._state = nullptr;
    other._task = nullptr;
    other._future = nullptr;
  }
  promise(const promise<T>&) = delete;

  template<typename Func>
  void sched_task(Func && f) {
    assert(_task == nullptr);
    auto t = new task<Func, T>(std::forward<Func>(f));
    _state = t->get_state();
    _task = t;
  }
  
  void set_value(T&& value) {
    _state->set(std::forward<T>(value));
    if (_task) {
      _task->run();
      _task = nullptr; 
    }
  }

  auto get_future() {
    assert(_future == nullptr); 
    return future<T>(this);
  }

  ~promise() {
    //std::cout << "promise destruct " << std::endl;
    if (_task) {
      delete _task;
    }
  }

};

template <typename T>
struct futurator {
  template <typename Func>
  static auto apply(Func &&f, T&& t) {
    return f(std::forward<T>(t));
  }

  template <typename Func>
  using Result_t = std::invoke_result_t<Func, T>;
};

template <typename ...Args>
struct futurator<std::tuple<Args...>> {
  template <typename Func>
  static auto apply(Func &&f, const std::tuple<Args...>& tuple) {
    return std::apply(std::forward<Func>(f), tuple);
  }

  template <typename Func>
  using Result_t = std::invoke_result_t<Func, Args...>;
};


template <typename T>
class future {
  future_state<T> _state;
  promise<T> *_pr;

public:
  friend class promise<T>;
  future() : _state(), _pr() {}
  future(T&& _state) : _state(std::forward<T>(_state)), _pr() {}
  future(promise<T> *pr) : _pr(pr) { _pr->_state = &_state; _pr->_future = this; }
  future(const future<T>&) = delete;
  future(future<T> &&other) {
    _state = std::move(other._state);
    _pr = other._pr;
    if (_pr) {
      _pr -> _future = this;
      _pr -> _state = &_state;
      other.detach();
    }
  }

  auto get_promise() {
    return promise<T>(this);
  }

  promise<T>* detach() {
    //detach
    _pr -> _state = nullptr;
    _pr -> _future = nullptr;
    return std::exchange(_pr, nullptr);
  }

  ~future() {
    //std::cout << "future destruct " << std::endl;
    if (_pr) {
      detach();
    }
  }

  void set_value(T && value) {
    _state.set(std::forward<T>(value));
  }


  future_state<T>* get_state() {
    if (_pr) {
      return _pr->_state;
    }
    return &_state;
  }

  bool available() { return get_state()->available(); } 
  T&& get() { return get_state()->get(); }

  template <typename Func>
  auto then(Func &&f) -> future<typename futurator<T>::Result_t<Func>> {
    using Res = typename futurator<T>::Result_t<Func>;

    future<Res> fut;

    if (available()) {
      fut.set_value(futurator<T>::apply(std::forward<Func>(f), _state.get()));
    } else {
      //this future is going to destruct
      //detach the promise with this future
      assert(_pr);
      //if f is a Func&, we move the f into the lambda.
      detach()->sched_task([res_pr = fut.get_promise(), func = std::move(f)](future_state<T> * _s) mutable {
          res_pr.set_value(futurator<T>::apply(func, _s->get()));
      });
    }
    return fut;
  }

};


int main() {
  promise<int> x;
  auto f = x.get_future();
  auto y = f.then([](int x) { return std::make_tuple(0, x * 3);}).then([](int x, int y) { return y * 2; });
  x.set_value(2);
  std::cout << y.get() << std::endl;
}


