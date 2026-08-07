cd ~/Desktop/projectNyar
nano nyar_master_test.py
Cole o código completo abaixo:
#!/usr/bin/env python3
import os
import sys
import subprocess
import time

# Cores para o terminal
BLUE = "\033[94m"
GREEN = "\033[92m"
YELLOW = "\033[93m"
RED = "\033[91m"
RESET = "\033[0m"

def print_header(title):
    print(f"\n{BLUE}======================================================={RESET}")
    print(f"      {title}")
    print(f"{BLUE}======================================================={RESET}\n")

def get_input(prompt, default=None):
    if default:
        res = input(f"{prompt} [{default}]: ").strip()
        return res if res else default
    return input(prompt).strip()

def main():
    print_header("Nyar Master: Orquestrador de Experimentos (1-Command)")

    # 1. IP da Raspberry Pi
    rasp_ssh = get_input("Qual o SSH da Raspberry Pi?", "tui@10.32.162.84")

    # 2. Nome do arquivo CSV
    csv_name = get_input("Qual o nome do arquivo para salvar os dados (ex: teste_c2)? ")
    if not csv_name:
        csv_name = "accel_statistics"
    if not csv_name.endswith(".csv"):
        csv_name += ".csv"

    # 3. Escolha do Cenário de Estresse
    print(f"\n{YELLOW}Selecione o cenário de estresse para este teste:{RESET}")
    print("1. Nenhum (Acelerômetro em condição normal - Baseline)")
    print("2. Estressar a Rede (Inundar o Wi-Fi via iperf3)")
    print("3. Estressar a Raspberry Pi (CPU da placa a 100% via stress-ng)")

    choice = get_input("Escolha uma opção (1/2/3)", "1")

    workstation_ip = ""
    if choice == "2":
        workstation_ip = get_input("Qual o IP desta Workstation na rede (para o iperf3)? ")

    local_iperf = None
    remote_i2c = None
    remote_stress = None

    try:
        # 4. Garantir que a Pi não tem processos antigos rodando
        print(f"{YELLOW}>> Limpando processos antigos na Raspberry Pi...{RESET}")
        subprocess.run(f"ssh {rasp_ssh} 'pkill -f i2c_server; pkill -f iperf3; pkill -f stress-ng'", shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        time.sleep(1)

        # 5. Iniciar o I2C Server Remotamente na Raspberry Pi em Background [1]
        print(f"{GREEN}>> Inicializando o Servidor I2C na Raspberry Pi...{RESET}")
        i2c_cmd = f"ssh {rasp_ssh} 'source ~/projectNyar/install/setup.bash && export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp && ros2 run i2c_edge_server i2c_server'"
        remote_i2c = subprocess.Popen(i2c_cmd, shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

        # Espera o DDS da placa se propagar
        time.sleep(2)

        # 6. Ativação dos Estressores
        if choice == "2":
            print_header("Ativando Estresse de Rede...")
            local_iperf = subprocess.Popen(["iperf3", "-s"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            time.sleep(1)

            stress_cmd = f"ssh {rasp_ssh} 'iperf3 -c {workstation_ip} -u -b 60M -t 200'"
            print(f"{YELLOW}>> Rodando iperf3 remoto...{RESET}")
            remote_stress = subprocess.Popen(stress_cmd, shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

        elif choice == "3":
            print_header("Ativando Estresse de CPU na Raspberry Pi...")
            stress_cmd = f"ssh {rasp_ssh} 'stress-ng --cpu 4 --timeout 200s'"
            print(f"{YELLOW}>> Rodando stress-ng remoto...{RESET}")
            remote_stress = subprocess.Popen(stress_cmd, shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

        else:
            print_header("Rodando em Modo Normal (Sem estresse)...")

        # 7. Execução do nó local na Workstation [3]
        print(f"{GREEN}>> Inicializando o cliente do acelerômetro na Workstation...{RESET}")
        print(f"{GREEN}>> Gravando dados em: {csv_name}{RESET}")
        print(f"{RED}>> Pressione CTRL+C para parar o teste e desligar as duas máquinas!{RESET}\n")

        ros_cmd = f"source install/setup.bash && export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp && ros2 run accel_logic_client vector_logic_client --ros-args -p csv_filename:={csv_name}"
        subprocess.run(ros_cmd, shell=True, executable="/bin/bash")

    except KeyboardInterrupt:
        print(f"\n{RED}>> Experimento finalizado pelo operador!{RESET}")
    finally:
        # 8. Limpeza Total de Ambos os Lados
        print_header("Orquestrando encerramento sincronizado...")

        if local_iperf:
            print(">> Desligando servidor iperf3 local...")
            local_iperf.terminate()
            local_iperf.wait()

        print(f"{YELLOW}>> Desligando nós e estressores na Raspberry Pi...{RESET}")
        # Envia comandos SSH diretos para limpar os binários de execução
        subprocess.run(f"ssh {rasp_ssh} 'pkill -f i2c_server; pkill -f iperf3; pkill -f stress-ng'", shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

        if remote_i2c:
            remote_i2c.wait()
        if remote_stress:
            remote_stress.wait()

        print(f"{GREEN}✔ Concluído! Tudo limpo e dados salvos em '{csv_name}'{RESET}\n")

if __name__ == "__main__":
    main()
