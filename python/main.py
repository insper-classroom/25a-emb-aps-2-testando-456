import sys
import glob
import serial
import pyautogui
import tkinter as tk
from tkinter import ttk
from tkinter import messagebox
from time import time

# Configura pyautogui para baixa latência
pyautogui.MINIMUM_DURATION = 0
pyautogui.MINIMUM_SLEEP = 0

# Estado das teclas/botões
estado = {
    'w': False,
    'a': False,
    's': False,
    'd': False,
    'e': False,
    'space': False,
    'shift': False,
    'left': False,
    'right': False
}

# Rastreia botões pressionados
botoes_pressionados = {}  # {gpio: timestamp}

def atualizar_tecla(tecla, ativo):
    """Pressiona ou solta uma tecla/clique."""
    if ativo and not estado[tecla]:
        pyautogui.keyDown(tecla)
        estado[tecla] = True
    elif not ativo and estado[tecla]:
        pyautogui.keyUp(tecla)
        estado[tecla] = False

def scroll_down():
    """Simula scroll down."""
    pyautogui.scroll(-1)

def parse_data(data):
    """Interpreta os dados recebidos: [tipo, id, valor_low, valor_high]."""
    tipo = data[0]
    id = data[1]
    valor = int.from_bytes(data[2:4], byteorder='little', signed=True)
    return tipo, id, valor

def controle(ser, status_label):
    """Lê eventos seriais e processa joysticks/botões."""
    global botoes_pressionados
    botoes = {
        15: 'left',      # BTN_LM
        14: 'right',     # BTN_RM
        13: None,        # BTN_SCROLL
        12: 'e',         # BTN_INVENTORY
        11: 'space',     # BTN_JUMP
        10: 'shift'      # BTN_RUN
    }

    while True:
        # Aguarda byte de sincronização
        sync_byte = ser.read(size=1)
        if not sync_byte or sync_byte[0] != 0xFF:
            continue
        # Lê 4 bytes: [tipo, id, valor_low, valor_high]
        data = ser.read(size=4)
        if len(data) < 4:
            continue
        tipo, id, valor = parse_data(data)
        status_label.config(text=f"Evento: tipo={tipo}, id={id}, valor={valor}")

        # Processa evento
        if tipo == 0:  # Joystick 1 (WASD)
            if id == 0:  # X
                atualizar_tecla('a', valor < -10)
                atualizar_tecla('d', valor > 10)
            elif id == 1:  # Y
                atualizar_tecla('w', valor < -10)
                atualizar_tecla('s', valor > 10)
        elif tipo == 1:  # Joystick 2 (mouse)
            if id == 0:  # X
                pyautogui.moveRel(valor, 0)
            elif id == 1:  # Y
                pyautogui.moveRel(0, valor)
        elif tipo == 2:  # Botões
            if valor in botoes:  # Pressionado
                botoes_pressionados[valor] = time()
                if valor == 13:
                    scroll_down()
                elif botoes[valor]:
                    atualizar_tecla(botoes[valor], True)
            elif valor == 0 and botoes_pressionados:  # Solto
                for gpio in list(botoes_pressionados):
                    if botoes[gpio]:
                        atualizar_tecla(botoes[gpio], False)
                    del botoes_pressionados[gpio]

        # Timeout pra soltar botões
        now = time()
        for gpio in list(botoes_pressionados):
            if now - botoes_pressionados[gpio] > 0.5:  # 500ms
                if gpio in botoes and botoes[gpio]:
                    atualizar_tecla(botoes[gpio], False)
                del botoes_pressionados[gpio]

def serial_ports():
    """Retorna lista de portas seriais disponíveis."""
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
    """Abre conexão serial e inicia leitura."""
    if not port_name:
        messagebox.showwarning("Aviso", "Selecione uma porta serial antes de conectar.")
        return

    try:
        ser = serial.Serial(port_name, 115200, timeout=1)
        status_label.config(text=f"Conectado em {port_name}", foreground="green")
        mudar_cor_circulo("green")
        botao_conectar.config(text="Conectado")
        root.update()

        # Inicia leitura
        controle(ser, status_label)

    except Exception as e:
        messagebox.showerror("Erro", f"Não foi possível conectar em {port_name}.\nErro: {e}")
        mudar_cor_circulo("red")
    finally:
        ser.close()
        status_label.config(text="Conexão encerrada.", foreground="red")
        mudar_cor_circulo("red")

def criar_janela():
    root = tk.Tk()
    root.title("Controle de Mouse")
    root.geometry("400x250")
    root.resizable(False, False)

    # Tema escuro
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

    # Frame principal
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

    # Frame inferior
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