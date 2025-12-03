#!/usr/bin/env python3

import sys
import glob
import serial
import pyautogui
import tkinter as tk
from tkinter import ttk
from tkinter import messagebox

# remove delay padrao para ficar mais rapido
pyautogui.PAUSE = 0  

def handle_input(axis, value):
    """
    mapeia os botoes e encoders para as teclas do jogo.
    """
    print(f"\nAcao detectada -> Eixo: {axis} | Valor: {value}")
    
    # 1: reset (aperto = r)
    if axis == 1:
        if value == 1:
            pyautogui.press('r')

    # 2: buzina (segura k)
    elif axis == 2:
        if value == 1:
            pyautogui.keyDown('k')
        else:
            pyautogui.keyUp('k')

    # 3: encoder tc/esc (ctrl+q)
    elif axis == 3:
        if value != 0:
            pyautogui.hotkey('ctrl', 'q')

    # 4: pisca alerta (aperta h na ida e na volta)
    elif axis == 4:
        pyautogui.press('h')
        

    # 5: farois (descida = nn, subida = n)
    elif axis == 5:
        if value == 1:
            pyautogui.press('n')
            pyautogui.press('n')
        elif value == 0:
            pyautogui.press('n')

    # 6: farol alto (segura j)
    elif axis == 6:
        if value == 1:
            pyautogui.keyDown('j')
        else:
            pyautogui.keyUp('j')

    # 7: ignicao (descida = 3, subida = 0) - invertido conforme solicitado
    elif axis == 7:
        if value == 1:
            pyautogui.press('3') # edge down
        elif value == 0:
            pyautogui.press('0') # edge up

    # 8: two step (aperta t na ida e na volta)
    elif axis == 8:
        pyautogui.press('t')

    # 9: nitro (segura b)
    elif axis == 9:
        if value == 1:
            pyautogui.keyDown('b')
        else:
            pyautogui.keyUp('b')

    # 10: encoder launch rpm (+/-)
    elif axis == 10:
        if value > 0:
            pyautogui.press('+')
        elif value < 0:
            pyautogui.press('-')

def parse_data(data):
    """
    converte os bytes recebidos para eixo e valor inteiro.
    """
    axis = data[0]
    # converte 2 bytes para inteiro com sinal (little endian)
    value = int.from_bytes(data[1:3], byteorder='little', signed=True)
    return axis, value

def controle(ser):
    """
    le a serial byte a byte e monta o pacote ate achar o 0xff no final.
    """
    pyautogui.FAILSAFE = False
    buffer = []

    while True:
        # le 1 byte
        byte = ser.read(1)
        
        # se nao chegou nada, continua tentando
        if not byte:
            # mantem a janela atualizada mesmo sem dados
            if 'root' in globals():
                root.update()
            continue

        # adiciona o byte lido ao buffer
        byte_int = byte[0]
        buffer.append(byte_int)

        # verifica se o ultimo byte eh o de sincronia (0xff)
        if buffer[-1] == 0xFF:
            # verifica se o pacote esta completo (3 dados + 1 sync = 4 bytes)
            if len(buffer) == 4:
                # pega os 3 primeiros bytes (dados)
                data_packet = bytes(buffer[:-1])
                try:
                    axis, value = parse_data(data_packet)
                    handle_input(axis, value)
                except Exception as e:
                    print(f"erro de parse: {e}")
                
                # limpa buffer para proxima leitura
                buffer = []
            
            # se tiver mais bytes que o esperado, algo desalinhou, limpa tudo
            elif len(buffer) > 4:
                buffer = []
        
        # protecao: se buffer crescer demais sem achar 0xff, limpa (ruido)
        if len(buffer) > 10:
            buffer = []

        # atualiza a gui
        if 'root' in globals():
            root.update()

def serial_ports():
    """
    lista portas seriais disponiveis.
    """
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
        
    result = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
        except (OSError, serial.SerialException):
            pass
    return result

def conectar_porta(port_name, botao_conectar, status_label, mudar_cor_circulo):
    """
    conecta na porta e inicia o loop.
    """
    if not port_name:
        messagebox.showwarning("Aviso", "Selecione uma porta serial.")
        return

    try:
        # timeout curto para nao travar a leitura
        ser = serial.Serial(port_name, 115200, timeout=0.05)
        
        status_label.config(text=f"Conectado: {port_name}", foreground="green")
        mudar_cor_circulo("green")
        botao_conectar.config(text="Rodando...", state="disabled")
        
        global root
        root.update()

        controle(ser)

    except Exception as e:
        messagebox.showerror("Erro", f"Falha na conexão: {e}")
        if 'ser' in locals() and ser.is_open:
            ser.close()
        status_label.config(text="Desconectado", foreground="red")
        mudar_cor_circulo("red")
        botao_conectar.config(text="Conectar", state="normal")

def criar_janela():
    global root
    root = tk.Tk()
    root.title("Controle BeamNG")
    root.geometry("600x300")
    root.resizable(False, False)

    # tema escuro
    dark_bg = "#2e2e2e"
    dark_fg = "#ffffff"
    accent_color = "#007acc"
    root.configure(bg=dark_bg)

    style = ttk.Style(root)
    style.theme_use("clam")
    style.configure("TFrame", background=dark_bg)
    style.configure("TLabel", background=dark_bg, foreground=dark_fg, font=("Segoe UI", 11))
    style.configure("TButton", font=("Segoe UI", 10, "bold"), foreground=dark_fg, background="#444444", borderwidth=0)
    style.map("TButton", background=[("active", "#555555")])
    style.configure("Accent.TButton", font=("Segoe UI", 12, "bold"), foreground=dark_fg, background=accent_color)
    style.map("Accent.TButton", background=[("active", "#005f9e")])
    style.configure("TCombobox", fieldbackground=dark_bg, background=dark_bg, foreground=dark_fg)

    frame_principal = ttk.Frame(root, padding="20")
    frame_principal.pack(expand=True, fill="both")

    lbl_titulo = ttk.Label(frame_principal, text="Interface Serial - BeamNG", font=("Segoe UI", 14, "bold"))
    lbl_titulo.pack(pady=(0, 20))

    porta_var = tk.StringVar()
    portas = serial_ports()
    if portas: porta_var.set(portas[0])

    combo = ttk.Combobox(frame_principal, textvariable=porta_var, values=portas, state="readonly", width=15)
    combo.pack(pady=5)

    btn_conectar = ttk.Button(
        frame_principal,
        text="Conectar",
        style="Accent.TButton",
        command=lambda: conectar_porta(porta_var.get(), btn_conectar, status_label, mudar_cor_circulo)
    )
    btn_conectar.pack(pady=20)

    # rodape
    footer = tk.Frame(root, bg=dark_bg)
    footer.pack(side="bottom", fill="x", padx=10, pady=10)

    status_label = tk.Label(footer, text="Aguardando...", font=("Segoe UI", 10), bg=dark_bg, fg="#aaaaaa")
    status_label.pack(side="left")

    canvas = tk.Canvas(footer, width=15, height=15, bg=dark_bg, highlightthickness=0)
    indicador = canvas.create_oval(2, 2, 13, 13, fill="red", outline="")
    canvas.pack(side="right")

    def mudar_cor_circulo(cor):
        canvas.itemconfig(indicador, fill=cor)

    root.mainloop()

if __name__ == "__main__":
    criar_janela()