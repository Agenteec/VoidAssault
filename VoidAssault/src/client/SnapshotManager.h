// client/SnapshotManager.h
#pragma once
#include "common/NetworkPackets.h"
#include "raymath.h"
#include <deque>
#include <map>
#include <algorithm>
#include <cmath>

class SnapshotManager {
public:
    std::deque<WorldSnapshotPacket> history;

    const double INTERPOLATION_DELAY = 0.020;

    float LerpAngle(float start, float end, float amount) {
        float difference = std::abs(end - start);
        if (difference > 180.0f) {
            if (end > start) start += 360.0f;
            else end += 360.0f;
        }
        float value = start + ((end - start) * amount);
        if (value >= 0 && value <= 360) return value;
        return fmodf(value, 360.0f);
    }

    void PushSnapshot(const WorldSnapshotPacket& snap) {
        if (!history.empty() && snap.serverTime <= history.back().serverTime) {
            return;
        }

        history.push_back(snap);

        while (history.size() > 2 && history[0].serverTime < history.back().serverTime - 1.0) {
            history.pop_front();
        }
    }


    bool GetInterpolatedState(uint32_t entityId, double clientRenderTime, EntityState& outState) {
        if (history.empty()) return false;


        auto it = std::lower_bound(history.begin(), history.end(), clientRenderTime,
            [](const WorldSnapshotPacket& snap, double val) {
                return snap.serverTime < val;
            });

        if (it == history.begin()) {
            return FindEntityInSnapshot(history.front(), entityId, outState);
        }

        if (it == history.end()) {
            return FindEntityInSnapshot(history.back(), entityId, outState);
        }

        const auto& snapB = *it;
        const auto& snapA = *(--it);

        EntityState entA, entB;
        bool hasA = FindEntityInSnapshot(snapA, entityId, entA);
        bool hasB = FindEntityInSnapshot(snapB, entityId, entB);

        if (hasA && hasB) {
            double totalDt = snapB.serverTime - snapA.serverTime;
            double currentDt = clientRenderTime - snapA.serverTime;
            float t = (float)(currentDt / totalDt);


            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;

            if (entB.type == EntityType::BULLET) {
                outState = entB;
                outState.position = Vector2Lerp(entA.position, entB.position, t);
            }
            else {
                outState = entB;
                outState.position = Vector2Lerp(entA.position, entB.position, t);
                outState.rotation = LerpAngle(entA.rotation, entB.rotation, t);
            }
            return true;
        }

        if (hasB) {
            outState = entB;
            return true;
        }

        return false;
    }

private:
    bool FindEntityInSnapshot(const WorldSnapshotPacket& snap, uint32_t id, EntityState& out) {
        for (const auto& e : snap.entities) {
            if (e.id == id) {
                out = e;
                return true;
            }
        }
        return false;
    }
};