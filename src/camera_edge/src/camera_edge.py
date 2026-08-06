import subprocess
from launch import LaunchDescription
from launch_ros.actions import Node

# --- O RADAR PROCURANDO NO BARRAMENTO DESSA P#@"! ---
def rastrear_picam():
    try:
        output = subprocess.check_output(['v4l2-ctl', '--list-devices'], text=True)
        linhas = output.split('\n')
        for i, linha in enumerate(linhas):
            if 'mmal service' in linha or 'bcm2835-v4l2' in linha:
                return linhas[i+1].strip()
    except Exception as e:
        print("Aviso Azathoth: O Deus falhou ao rastrear o vassalo PiCam! Usando porta de segurança.")
    return '/dev/video2'

# --- A DIVINDADE DE AZATHOTH INVADINDO O TERMINAL ---
def perguntar(pergunta):
    # Pausa e espera a resposta. Só retorna True se o tolo usuário digitar 'y' ou 'Y'
    resposta = input(f"{pergunta} [y/N]: ").strip().lower()
    return resposta == 'y'

def generate_launch_description():
    print("\n=======================================================")
    print("      Azathoth Vision: Painel de Controle de Energia        ")
    print("=======================================================\n")

    # Faz o glorioso interrogatório tático no terminal
    ligar_ocam = perguntar("Quer que eu invoque a oCam?")
    ligar_zed = perguntar("Quer que eu invoque a ZED?")
    ligar_picam = perguntar("Quer que eu invoque a PiCam?")
    print("\n=======================================================\n")

    # A lista vazia de soldados. Só entra quem foi convocado pelo grande Azathoth!
    nos_para_iniciar = []

    if ligar_ocam:
        print(">> Invocando motor da oCam...")
        nos_para_iniciar.append(
            Node(
                package='v4l2_camera',
                executable='v4l2_camera_node',
                name='ocam_node',
                namespace='ocam',
                parameters=[{
                    'video_device': '/dev/v4l/by-id/usb-WITHROBOT_Inc._oCam-1MGN-U_SN_27425114-video-index0',
                    'image_size': [640, 480],
                    'camera_frame_id': 'ocam_link'
                }]
            )
        )

    if ligar_zed:
        print(">> Invocando motor da ZED...")
        nos_para_iniciar.append(
            Node(
                package='v4l2_camera',
                executable='v4l2_camera_node',
                name='zed_node',
                namespace='zed',
                parameters=[{
                    'video_device': '/dev/v4l/by-id/usb-Technologies__Inc._ZED-video-index0',
                    'image_size': [1344, 384],
                    'camera_frame_id': 'zed_link'
                }]
            )
        )

    if ligar_picam:
        print(">> Invocando radar e motor da PiCam...")
        porta_picam_blindada = rastrear_picam()
        nos_para_iniciar.append(
            Node(
                package='v4l2_camera',
                executable='v4l2_camera_node',
                name='picam_node',
                namespace='picam',
                parameters=[{
                    'video_device': porta_picam_blindada,
                    'image_size': [640, 480],
                    'camera_frame_id': 'picam_link'
                }]
            )
        )

    # Se todas as respostas foram N(dai ta pedindo né)
    if not nos_para_iniciar:
        print("AVISO: Nenhuma câmera selecionada. Azathoth está cego!(Que ironia né)")

    # Retorna para o ROS 2 apenas o exército selecionado
    return LaunchDescription(nos_para_iniciar)
