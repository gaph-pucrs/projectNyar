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
    print_header("Nyar Statistics: Painel de Automação de Experimentos")

    # 1. Escolha do Nome do Arquivo CSV
    csv_name = get_input("Qual o nome do arquivo para salvar os dados (ex: teste_c2)? ")
    if not csv_name:
        csv_name = "accel_statistics"
    if not csv_name.endswith(".csv"):
        csv_name += ".csv"

    # 2. Escolha do Cenário de Estresse
    print(f"\n{YELLOW}Selecione o cenário de estresse para este teste:{RESET}")
    print("1. Nenhum (Acelerômetro em condição normal - Baseline)")
    print("2. Estressar a Rede (Inundar o Wi-Fi via iperf3)")
    print("3. Estressar a Raspberry Pi (Levar a CPU da placa a 100% via stress-ng)")

    choice = get_input("Escolha uma opção (1/2/3)", "1")

    ssh_target = None
    local_iperf = None
    remote_process = None

    # 3. Se houver estresse remoto, precisamos do SSH da placa
    if choice in ["2", "3"]:
        ssh_target = get_input("Qual o SSH para conectar na Rasp (ex: tui@10.32.162.84 ou tui@100.77.33.22)", "tui@10.32.162.84")

        if choice == "2":
            workstation_ip = get_input("Qual o IP da Workstation (PC) na rede (para o iperf3)? ")

    try:
        # 4. Ativação dos Estressores
        if choice == "2":
            print_header("Ativando Estresse de Rede...")
            # Inicia servidor iperf3 local na Workstation em background
            local_iperf = subprocess.Popen(["iperf3", "-s"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            time.sleep(1) # Espera o servidor subir

            # Dispara cliente iperf3 remoto na Pi via SSH
            cmd = f"ssh {ssh_target} 'iperf3 -c {workstation_ip} -u -b 60M -t 200'"
            print(f"{YELLOW}>> Executando remotamente na Pi: {cmd}{RESET}")
            remote_process = subprocess.Popen(cmd, shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

        elif choice == "3":
            print_header("Ativando Estresse de CPU na Raspberry Pi...")
            # Dispara o stress-ng na Raspberry Pi para estressar os cores da placa
            cmd = f"ssh {ssh_target} 'stress-ng --cpu 4 --timeout 200s'"
            print(f"{YELLOW}>> Executando remotamente na Pi: {cmd}{RESET}")
            remote_process = subprocess.Popen(cmd, shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

        else:
            print_header("Iniciando Experimento Normal (Sem estresse artificial)...")

        # 5. Execução do nó ROS 2 com o Parâmetro customizado do CSV
        print(f"{GREEN}>> Inicializando o nó de lógica do acelerômetro...{RESET}")
        print(f"{GREEN}>> Os dados serão gravados fisicamente em: {csv_name}{RESET}")
        print(f"{RED}>> Pressione CTRL+C para encerrar o experimento.{RESET}\n")

        # Executa o colcon build implicitamente antes ou assume que já está compilado.
        # Roda o nó injetando o parâmetro correspondente
        ros_cmd = f"source install/setup.bash && ros2 run accel_logic_client vector_logic_client --ros-args -p csv_filename:={csv_name}"
        subprocess.run(ros_cmd, shell=True, executable="/bin/bash")

    except KeyboardInterrupt:
        print(f"\n{RED}>> Experimento interrompido pelo usuário!{RESET}")
    finally:
        # 6. Cleanup Geral Garantido
        print_header("Limpando processos e restaurando ambiente...")

        if local_iperf:
            print(">> Parando servidor iperf3 local...")
            local_iperf.terminate()
            local_iperf.wait()

        if ssh_target:
            print(">> Finalizando estressores remotos na Raspberry Pi...")
            # Envia comando SSH rápido para matar qualquer iperf3 ou stress-ng órfão na placa
            subprocess.run(f"ssh {ssh_target} 'pkill -f iperf3; pkill -f stress-ng'", shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

        print(f"{GREEN}✔ Pronto! O teste foi encerrado e os dados estão salvos em: '{csv_name}'{RESET}\n")

if __name__ == "__main__":
    main()
