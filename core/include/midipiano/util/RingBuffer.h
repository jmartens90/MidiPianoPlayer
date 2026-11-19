#ifndef MIDIPIANO_UTIL_RINGBUFFER_H
#define MIDIPIANO_UTIL_RINGBUFFER_H

#include <cstddef>
#include <vector>

namespace midipiano::util {

// Vereiste 34: 34.useful template function or class
// RingBuffer<T> onthoudt de laatste N waarden van eender welk type. De synth gebruikt
// RingBuffer<float> voor de laatste 1024 samples die de oscilloscoop tekent; dezelfde klasse kan
// later RingBuffer<MidiMessage> zijn voor een MIDI-log, zonder een regel code te kopieren.
template <typename T>
class RingBuffer {
 public:
  // Vereiste 41: 41.dynamic memory allocation (new)
  // De grootte is pas bij het aanmaken bekend (de scope wil 1024 samples, een test 4), dus de
  // buffer wordt op de heap aangemaakt met new[].
  explicit RingBuffer(std::size_t capacity) : data_(new T[capacity]{}), capacity_(capacity) {}

  // Vereiste 42: 42.dynamic memory removing (delete or kill children)
  // Wat met new[] werd aangemaakt, wordt hier met delete[] vrijgegeven. Zonder deze destructor zou
  // elke weggegooide RingBuffer zijn geheugen lekken.
  ~RingBuffer() { delete[] data_; }

  // Een kopie zou dezelfde pointer delen en hem twee keer deleten, dus kopieren is verboden.
  RingBuffer(const RingBuffer&) = delete;
  RingBuffer& operator=(const RingBuffer&) = delete;

  void push(const T& value) {
    data_[head_] = value;
    head_ = (head_ + 1) % capacity_;
    if (size_ < capacity_) ++size_;
  }

  // Kopieert de inhoud van oud naar nieuw in out.
  void copyTo(std::vector<T>& out) const {
    out.resize(size_);
    const std::size_t start = (head_ + capacity_ - size_) % capacity_;
    for (std::size_t i = 0; i < size_; ++i) out[i] = data_[(start + i) % capacity_];
  }

  std::size_t size() const { return size_; }
  std::size_t capacity() const { return capacity_; }

 private:
  T* data_;
  std::size_t capacity_;
  std::size_t head_ = 0;
  std::size_t size_ = 0;
};

}  // namespace midipiano::util

#endif  // MIDIPIANO_UTIL_RINGBUFFER_H
