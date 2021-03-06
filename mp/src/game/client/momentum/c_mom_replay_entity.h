#pragma once

#include "c_mom_ghost_base.h"

class C_MomentumReplayGhostEntity : public C_MomentumGhostBaseEntity
{
    DECLARE_CLASS(C_MomentumReplayGhostEntity, C_MomentumGhostBaseEntity);
    DECLARE_CLIENTCLASS();
    DECLARE_INTERPOLATION()

  public:
    C_MomentumReplayGhostEntity();

    bool IsReplayGhost() const OVERRIDE { return true; }

    CNetworkVar(bool, m_bIsPaused);  // Is the replay paused?
    CNetworkVar(int, m_iCurrentTick);// The current tick of the replay playback
    CNetworkVar(int, m_iTotalTicks); // Total ticks in the replay (run time + start zone + end zone)

    // Run entity stuff
    float GetCurrentRunTime() OVERRIDE;
    RUN_ENT_TYPE GetEntType() OVERRIDE { return RUN_ENT_REPLAY; }
};