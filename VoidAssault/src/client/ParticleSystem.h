#pragma once
#include "raylib.h"
#include <vector>
#include <algorithm>

struct Particle {
    Vector2 position;
    Vector2 velocity;
    float life;      // 1.0 -> 0.0
    float decayRate; // Скорость исчезновения
    float size;
    Color color;
};

class ParticleSystem {
    std::vector<Particle> particles;

public:
    void Spawn(Vector2 pos, Vector2 vel, Color col, float size, float lifeTime) {
        Particle p;
        p.position = pos;
        p.velocity = vel;
        p.color = col;
        p.size = size;
        p.life = 1.0f;
        p.decayRate = 1.0f / lifeTime;
        particles.push_back(p);
    }

    void SpawnExplosion(Vector2 pos, int count, Color col) {
        for (int i = 0; i < count; i++) {
            float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
            float speed = (float)GetRandomValue(50, 200) / 10.0f;
            Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
            Spawn(pos, vel, col, (float)GetRandomValue(3, 8), 0.5f);
        }
    }

    void Update(float dt) {
        for (auto& p : particles) {
            p.position.x += p.velocity.x * dt * 60.0f;
            p.position.y += p.velocity.y * dt * 60.0f;
            p.life -= p.decayRate * dt;
            p.size *= 0.95f;
        }

        particles.erase(
            std::remove_if(particles.begin(), particles.end(),
                [](const Particle& p) { return p.life <= 0; }),
            particles.end());
    }

    void Draw() {
        for (const auto& p : particles) {
            Color c = p.color;
            c.a = (unsigned char)(p.life * 255);
            DrawCircleV(p.position, p.size, c);
        }
    }
};