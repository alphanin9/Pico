#pragma once

/**
 * \brief Initialize anti-cheat bootstrapping.
 *
 * Should only be called once, but we still make sure it is only called once.
 */
extern "C" void Pico_Init();

/**
 * \brief Anti-cheat tick function.
 *
 * Call this in every frame of the game's main loop.
 */
extern "C" void Pico_PollMainLoop();

/**
 * \brief Tells the anti-cheat engine to uninitialize.
 */
extern "C" void Pico_Teardown();
