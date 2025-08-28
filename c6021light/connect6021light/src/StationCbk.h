#ifndef __STATIONCBK_H__
#define __STATIONCBK_H__

#include <hal/HalBase.h>
#include "RR32Can/callback/AccessoryCbk.h"

/*
 * \brief Class AccessoryCbk
 */
class AccessoryCbk : public RR32Can::callback::AccessoryCbk {
 public:
  void begin(hal::HalBase& hal);

  /**
   * \brief Called when an accessory packet was received.
   */
  void OnAccessoryPacket(RR32Can::TurnoutPacket& packet) override;

 private:
  hal::HalBase* hal = nullptr;
};

#endif  // __STATIONCBK_H__
