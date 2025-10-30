from beamngpy import BeamNGpy, Scenario, Vehicle
import time

# Conecta ao BeamNG (precisa estar com o jogo aberto)
bng = BeamNGpy('localhost', 64256)
bng.open()

# Cria veículo e cenário simples
vehicle = Vehicle('ego_vehicle', model='etk800', licence='GEARTEST')
scenario = Scenario('smallgrid', 'gear_test')
scenario.add_vehicle(vehicle, pos=(0, 0, 0), rot=(0, 0, 0))
scenario.make(bng)

bng.load_scenario(scenario)
bng.start_scenario()

# Loop para imprimir a marcha atual
try:
    while True:
        state = vehicle.state
        gear = state['gear']      # marcha atual
        print(f"Marcha atual: {gear}")
        time.sleep(0.2)           # atualiza a cada 0.2s
except KeyboardInterrupt:
    print("\nEncerrado pelo usuário.")
finally:
    bng.close()
