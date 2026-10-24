#pragma once
// Lo que sobrevive a un reinicio (apagón, OTA o Reiniciar). La 7B lo guarda
// en su NVS; aquí solo se dice qué se guarda, cuándo y qué se cree al leerlo.
//
// Cada campo va en su propia llave con su tamaño exacto: si un firmware nuevo
// cambia un campo, solo ese regresa a su valor de fábrica y lo demás sigue.
// Antes de escribir se compara con lo guardado, para no gastar la flash.
//
// En la 7B (documento, pág. 39: los litros se escriben cuando para el agua o
// cada 10 min, nunca por litro):
//   al arrancar          core::load(s, nvs); core::mixer_after_restart(s);
//   al tocar un ajuste   core::save(s, nvs, SaveGroup::settings) y counters
//   cada 10 min, o cuando para el agua: counters e history
//   antes de reiniciar   los tres

#include <cstddef>
#include <cstdint>

#include "system_state.h"

namespace core {

// Dónde se guarda: la NVS en la 7B, un mapa en las pruebas
class Store {
public:
    virtual ~Store() = default;
    // Copia a data lo guardado en key; false si no hay nada o si mide otra cosa
    virtual bool read(const char* key, void* data, std::size_t size) = 0;
    // false si no se pudo escribir
    virtual bool write(const char* key, const void* data, std::size_t size) = 0;
};

// Qué tan seguido cambia cada grupo
enum class SaveGroup : uint8_t {
    settings,  // calibraciones y ajustes: cambian al tocarlos
    counters,  // reloj, litros, cartuchos, eventos y bomba R
    history,   // gráficas e historial
};
inline constexpr SaveGroup kSaveGroups[] = {SaveGroup::settings, SaveGroup::counters, SaveGroup::history};

// Lo guardado encima del estado de fábrica. Lo que no tenga sentido (flash
// dañada, un campo de otra versión) regresa a su valor de fábrica. Regresa
// cuántas llaves leyó: 0 es la primera vez, o que se perdió lo guardado.
int load(SystemState& s, Store& store);

// Guarda lo de ese grupo que cambió. Regresa cuántas llaves escribió, o -1 si
// alguna no se pudo escribir.
int save(const SystemState& s, Store& store, SaveGroup group);

}  // namespace core
