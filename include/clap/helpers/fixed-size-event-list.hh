#pragma once

#include <array>
#include <cstdint>
#include <cstring>

#include <clap/events.h>

namespace clap { namespace helpers {

   template <uint32_t listSize, uint32_t maxEventSize>
   class FixedSizeEventList {
   public:
      FixedSizeEventList() = default;

      FixedSizeEventList(const FixedSizeEventList &) = delete;
      FixedSizeEventList(FixedSizeEventList &&) = delete;
      FixedSizeEventList &operator=(const FixedSizeEventList &) = delete;
      FixedSizeEventList &operator=(FixedSizeEventList &&) = delete;

      clap_event_header *tryAllocate(size_t size) {
         if (size > maxEventSize) {
            return nullptr;
         }

         // ensure we have space to store into the vector
         if (events.size() == eventsSize) {
            return nullptr;
         }

         auto ptr = dataPtr;
         if (ptr + size > dataBegin + listSize * maxEventSize) {
            return nullptr;
         }

         events[eventsSize++] = ptr - dataBegin;
         dataPtr += size;
         auto hdr = reinterpret_cast<clap_event_header *>(ptr);
         hdr->size = size;
         return hdr;
      }

      bool tryPush(const clap_event_header *h) {
         auto ptr = tryAllocate(h->size);
         if (!ptr)
            return false;

         std::memcpy(ptr, h, h->size);
         return true;
      }

      clap_event_header *get(uint32_t index) const {
         const auto offset = events.at(index);
         void *const ptr = dataBegin + offset;
         return static_cast<clap_event_header *>(ptr);
      }

      size_t size() const { return eventsSize; }

      bool empty() const { return eventsSize == 0; }

      void clear() {
         dataPtr = dataBegin;
         std::fill(events.begin(), events.end(), 0);
         eventsSize = 0;
      }

      const clap_input_events *clapInputEvents() const noexcept { return &inputEvents; }

      const clap_output_events *clapOutputEvents() const noexcept { return &outputEvents; }

   private:
      static uint32_t clapSize(const struct clap_input_events *list) {
         auto *self = static_cast<const FixedSizeEventList *>(list->ctx);
         return self->size();
      }

      static const clap_event_header_t *clapGet(const struct clap_input_events *list,
                                                uint32_t index) {
         auto *self = static_cast<const FixedSizeEventList *>(list->ctx);
         return self->get(index);
      }

      static bool clapPushBack(const struct clap_output_events *list,
                               const clap_event_header_t *event) {
         auto *self = static_cast<FixedSizeEventList *>(list->ctx);
         return self->tryPush(event);
      }

      const clap_input_events inputEvents = {this, &clapSize, &clapGet};
      const clap_output_events outputEvents = {this, &clapPushBack};

      std::array<uint8_t, listSize * maxEventSize> data{};
      uint8_t *const dataBegin = data.data();
      uint8_t *dataPtr = data.data();
      std::array<uint32_t, listSize> events{};
      uint32_t eventsSize = 0;
   };

}} // namespace clap::helpers
