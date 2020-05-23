#ifndef __STATIONCBK_H__
#define __STATIONCBK_H__

#include "RR32Can/StationCbk.h"

/*
 * \brief Class StationCbk
 */
class StationCbk : public RR32Can::StationCbk {
 public:
  RR32Can::Locomotive* getLoco(RR32Can::Locomotive::Uid_t uid) override { return nullptr; };

  /**
   * \brief Set the velocity of the loco with the given UID.
   *
   * Should have no effect, if the engine is not known.
   */
  void setLocoVelocity(RR32Can::Locomotive::Uid_t engineUid,
                       RR32Can::Velocity_t velocity) override{};

  /**
   * \brief Unconditionally set the velocity of the currently controlled engine
   * to 0.
   */
  void setLocoVelocity(RR32Can::Velocity_t velocity) override{};

  /**
   * \brief Set whether the system is on (true) or off (false)
   */
  void setSystemState(bool onOff) override{};

  /**
   * \brief Called when an accessory packet was received.
   */
  void OnAccessoryPacket(RR32Can::TurnoutPacket& packet) override;
};

#endif  // __STATIONCBK_H__
