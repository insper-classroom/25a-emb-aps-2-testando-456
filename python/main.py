import sys
import glob
import serial
import pyautogui
import tkinter as tk
from tkinter import ttk
from tkinter import messagebox
from time import time, sleep

pyautogui.MINIMUM_DURATION = 0
pyautogui.MINIMUM_SLEEP = 0

estado = {
    'w': False,
    'a': False,
    's': False,
    'd': False
}

botoes_pressionados = {}
gravando = False
reproduzindo = False
macro = []
inicio_gravacao = 0

def atualizar_tecla(tecla, ativo):
    if ativo and not estado[tecla]:
        pyautogui.keyDown(tecla)
        estado[tecla] = True
    elif not ativo and estado[tecla]:
        pyautogui.keyUp(tecla)
        estado[tecla] = False

def reproduzir_macro(status_label, mudar_cor_circulo):
    global reproduzindo
    if not macro:
        return
    reproduzindo = True
    status_label.config(text="Reproduzindo macro...")
    mudar_cor_circulo("blue")
    
    start_time = time()
    for evento in macro:
        tempo, tipo, id, valor = evento
        # Aguarda até o momento correto
        while time() - start_time < tempo:
            sleep(0.01)
        # Executa ação
        if tipo == 1:  # Joystick (mouse)
            if id == 0:
                pyautogui.moveRel(valor // 4, 0)
            elif id == 1:
                pyautogui.moveRel(0, valor // 4)
        elif tipo == 2:  # Botões (WASD)
            botoes = {14: 'w', 15: 'a', 20: 's', 21: 'd'}
            if valor in botoes:
                atualizar_tecla(botoes[valor], True)
            elif valor == 0:
                for tecla in estado:
                    if estado[tecla]:
                        atualizar_tecla(tecla, False)
    
    # Limpa teclas ativas e volta ao modo normal
    for tecla in estado:
        if estado[tecla]:
            atualizar_tecla(tecla, False)
    reproduzindo = False
    status_label.config(text="Conectado.")
    mudar_cor_circulo("green")

def parse_data(data):
    tipo = data[0]
    id = data[1]
    valor = int.from_bytes(data[2:4], byteorder='little', signed=True)
    return tipo, id, valor

def controle(ser, status_label, mudar_cor_circulo):
    global botoes_pressionados, gravando, reproduzindo, macro, inicio_gravacao
    botoes = {
        14: 'w',  # GP14
        15: 'a',  # GP15
        20: 's',  # GP20
        21: 'd',  # GP21
        22: None  # GP22 (macro)
    }

    while True:
        try:
            sync_byte = ser.read(size=1)
            if not sync_byte or sync_byte[0] != 0xFF:
                continue
            data = ser.read(size=4)
            if len(data) < 4:
                continue
            tipo, id, valor = parse_data(data)
            status_label.config(text=f"Evento: tipo={tipo}, id={id}, valor={valor}")

            # Debug
            if tipo == 1:
                print(f"Joystick: eixo={id}, valor={valor}")
            elif tipo == 2:
                print(f"Botao: valor={valor}, acao={'pressionar' if valor in botoes else 'soltar'}")

            # Gravação
            if gravando and tipo in [1, 2] and valor != 22:  # Exclui botão macro
                tempo = time() - inicio_gravacao
                macro.append([tempo, tipo, id, valor])

            # Ignora eventos durante reprodução
            if reproduzindo:
                continue

            if tipo == 1:  # Joystick (mouse)
                if id == 0:  # X
                    pyautogui.moveRel(valor // 4, 0)
                elif id == 1:  # Y
                    pyautogui.moveRel(0, valor // 4)
            elif tipo == 2:  # Botões
                if valor == 22:  # Botão macro
                    if not botoes_pressionados.get(22):  # Pressionar
                        botoes_pressionados[22] = time()
                        if gravando:
                            # Para gravação e inicia reprodução
                            gravando = False
                            reproduzir_macro(status_label, mudar_cor_circulo)
                        else:
                            # Inicia gravação
                            gravando = True
                            macro = []
                            inicio_gravacao = time()
                            status_label.config(text="Gravando macro...")
                            mudar_cor_circulo("orange")
                elif valor in botoes:
                    botoes_pressionados[valor] = time()
                    if botoes[valor]:
                        atualizar_tecla(botoes[valor], True)
                elif valor == 0 and botoes_pressionados:
                    for gpio in list(botoes_pressionados):
                        if botoes.get(gpio) and gpio != 22:
                            atualizar_tecla(botoes[gpio], False)
                        if gpio != 22:  # Mantém 22 até timeout
                            del botoes_pressionados[gpio]

            # Timeout para botões
            now = time()
            for gpio in list(botoes_pressionados):
                if now - botoes_pressionados[gpio] > 0.5:
                    if gpio in botoes and botoes[gpio]:
                        atualizar_tecla(botoes[gpio], False)
                    del botoes_pressionados[gpio]

        except KeyboardInterrupt:
            break
        except Exception as e:
            print(f"Erro no controle: {e}")
            break

def serial_ports():
    ports = []
    if sys.platform.startswith('win'):
        for i in range(1, 256):
            port = f'COM{i}'
            try:
                s = serial.Serial(port)
                s.close()
                ports.append(port)
            except (OSError, serial.SerialException):
                pass
    elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
        ports = glob.glob('/dev/tty[A-Za-z]*')
    elif sys.platform.startswith('darwin'):
        ports = glob.glob('/dev/tty.*')
    else:
        raise EnvironmentError('Plataforma não suportada.')
    
    result = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
        except (OSError, serial.SerialException):
            pass
    return result

def conectar_porta(port_name, root, botao_conectar, status_label, mudar_cor_circulo):
    if not port_name:
        messagebox.showwarning("Aviso", "Selecione uma porta serial antes de conectar.")
        return

    try:
        ser = serial.Serial(port_name, 115200, timeout=1)
        status_label.config(text=f"Conectado em {port_name}")
        mudar_cor_circulo("green")
        botao_conectar.config(text="Conectado")
        root.update()
        controle(ser, status_label, mudar_cor_circulo)
    except Exception as e:
        messagebox.showerror("Erro", f"Não foi possível conectar em {port_name}.\nErro: {e}")
        mudar_cor_circulo("red")
    finally:
        ser.close()
        status_label.config(text="Conexão encerrada.")
        mudar_cor_circulo("red")

def criar_janela():
    root = tk.Tk()
    root.title("Controle de Mouse")
    root.geometry("400x250")
    root.resizable(False, False)

    dark_bg = "#2e2e2e"
    dark_fg = "#ffffff"
    accent_color = "#007acc"
    root.configure(bg=dark_bg)

    style = ttk.Style(root)
    style.theme_use("clam")
    style.configure("TFrame", background=dark_bg)
    style.configure("TLabel", background=dark_bg, foreground=dark_fg, font=("Segoe UI", 11))
    style.configure("TButton", font=("Segoe UI", 10, "bold"),
                    foreground=dark_fg, background="#444444", borderwidth=0)
    style.map("TButton", background=[("active", "#555555")])
    style.configure("Accent.TButton", font=("Segoe UI", 12, "bold"),
                    foreground=dark_fg, background=accent_color, padding=6)
    style.map("Accent.TButton", background=[("active", "#005f9e")])
    style.configure("TCombobox", fieldbackground=dark_bg, background=dark_bg, foreground=dark_fg, padding=4)
    style.map("TCombobox", fieldbackground=[("readonly", dark_bg)])

    frame_principal = ttk.Frame(root, padding="20")
    frame_principal.pack(expand=True, fill="both")

    titulo_label = ttk.Label(frame_principal, text="Controle de Mouse", font=("Segoe UI", 14, "bold"))
    titulo_label.pack(pady=(0, 10))

    porta_var = tk.StringVar(value="")

    botao_conectar = ttk.Button(
        frame_principal,
        text="Conectar e Iniciar Leitura",
        style="Accent.TButton",
        command=lambda: conectar_porta(porta_var.get(), root, botao_conectar, status_label, mudar_cor_circulo)
    )
    botao_conectar.pack(pady=10)

    footer_frame = tk.Frame(root, bg=dark_bg)
    footer_frame.pack(side="bottom", fill="x", padx=10, pady=(10, 0))

    status_label = tk.Label(footer_frame, text="Aguardando seleção de porta...", font=("Segoe UI", 11),
                            bg=dark_bg, fg=dark_fg)
    status_label.grid(row=0, column=0, sticky="w")

    portas_disponiveis = serial_ports()
    if portas_disponiveis:
        porta_var.set(portas_disponiveis[0])
    port_dropdown = ttk.Combobox(footer_frame, textvariable=porta_var,
                                 values=portas_disponiveis, state="readonly", width=10)
    port_dropdown.grid(row=0, column=1, padx=10)

    circle_canvas = tk.Canvas(footer_frame, width=20, height=20, highlightthickness=0, bg=dark_bg)
    circle_item = circle_canvas.create_oval(2, 2, 18, 18, fill="red", outline="")
    circle_canvas.grid(row=0, column=2, sticky="e")

    footer_frame.columnconfigure(1, weight=1)

    def mudar_cor_circulo(cor):
        circle_canvas.itemconfig(circle_item, fill=cor)

    root.mainloop()

if __name__ == "__main__":
    criar_janela()