# ORBIXION

A physics-based 2D rocket launch simulator built in C with raylib. Choose Earth, Mars, or the Moon and launch a rocket governed by real orbital mechanics.

**[Play the downloadable build on itch.io →](https://hajer-chetoui.itch.io/orbixion)**

---

## About

Orbixion is an educational rocket launch simulator that lets you explore how a rocket behaves when launching from Earth, Mars, or the Moon. Instead of using scripted animations, the simulation is driven by real physics based on simplified Newtonian mechanics.

As the rocket climbs, the simulator continuously calculates the effects of gravity, engine thrust, atmospheric drag, fuel consumption, and the rocket's changing mass, letting you see how each factor influences the flight in real time.

## Controls

| Key | Action |
|---|---|
| `SPACE` | Fire the engine (consumes fuel) |
| `R` | Reset the simulation |
| `ESC` | Return to planet selection |
| `F11` | Toggle fullscreen |
| `ENTER` | Confirm menu selections |

## The physics

Every frame, the simulator:

1. Calculates gravitational acceleration: `g(h) = μ / (R + h)²`
2. Calculates the rocket's current weight
3. Calculates atmospheric drag (where applicable): `F_drag = ½ρ(h)v²C_dA`
4. Computes the net force acting on the rocket
5. Calculates acceleration via Newton's Second Law: `F = ma`
6. Updates velocity and altitude
7. Burns fuel and updates mass, using `ṁ = F_thrust / (Isp · g₀)`
8. Recalculates remaining delta-v via the **Tsiolkovsky rocket equation**: `Δv = Isp · g₀ · ln(m₀/m_dry)`

Each planet (Earth, Mars, Moon) uses its own real gravitational parameter, radius, and atmospheric density — so the same rocket genuinely behaves differently on each body, rather than just reskinning the same flight.

| Body | Surface gravity | Escape velocity | Atmosphere |
|---|---|---|---|
| Earth | 9.81 m/s² | ~11,186 m/s | Yes (thick) |
| Mars | 3.71 m/s² | ~5,027 m/s | Yes (thin) |
| Moon | 1.62 m/s² | ~2,375 m/s | None |

## Built with

- **C** (C11)
- **[raylib](https://www.raylib.com/)** for rendering and windowing
- **vcpkg** for dependency management

## Building from source

1. Install [vcpkg](https://github.com/microsoft/vcpkg) and integrate it with Visual Studio:
   ```
   git clone https://github.com/microsoft/vcpkg.git
   cd vcpkg
   .\bootstrap-vcpkg.bat
   .\vcpkg integrate install
   ```
2. Install raylib:
   ```
   .\vcpkg install raylib:x64-windows
   ```
3. Open this project in Visual Studio, set the platform to **x64**, and build.
4. Run ( the app looks for `rocket_icon.png` and `bahnschrift.ttf` in the same folder as the executable at runtime (both included in this repo)).

## Screenshots

<img width="892" height="686" alt="Capture d’écran 2026-08-01 224817" src="https://github.com/user-attachments/assets/76a92ea9-aeb1-4034-9254-d167be76a82f" />
<img width="892" height="690" alt="Capture d’écran 2026-08-01 224839" src="https://github.com/user-attachments/assets/0fd80586-a442-47c8-8d3f-d0aded6d965c" />
<img width="892" height="692" alt="Capture d’écran 2026-08-01 224910" src="https://github.com/user-attachments/assets/0a072723-15e2-4a02-b3de-6e7334620111" />

## License

This project is released under the [MIT License](LICENSE).

---

Built by Hajer Chetoui
