import tkinter as tk
from PIL import Image, ImageTk
import serial
import serial.tools.list_ports

# seleccion automatica puerto serial de la esp
ports = serial.tools.list_ports.comports()
for onePort in ports:
    if "Silicon Labs" in str(onePort):
        com = str(onePort).split(' ')
# config puerto serial
s_port = serial.Serial()
s_port.port = com[0]
s_port.baudrate = 115200
s_port.open()

# funcion del boton Go
def callback_go():
    global cnt, s_port
    s_port.flushInput()

    send_text = 'r' + str(ro_slider.get()) + 'a' + str(alpha_slider.get()) + ';'
    
    s_port.write(send_text.encode('ascii'))
    print(send_text)

# callback control manual
def callback_manual():
    global control_manual, manual_button, up_button, down_button, right_button, left_button
    control_manual = not control_manual
    
    if control_manual:
        manual_button.config(bg='#282c34', fg="#FDFDFF")
        up_button.config(state='active')
        down_button.config(state='active')
        right_button.config(state='active')
        left_button.config(state='active')
        calibrate_button.config(state='active')
        go.config(state='disabled')

    else:
        manual_button.config(bg="#FDFDFF", fg='#282c34')
        up_button.config(state='disabled')
        down_button.config(state='disabled')
        right_button.config(state='disabled')
        left_button.config(state='disabled')
        calibrate_button.config(state='disabled')
        go.config(state='active')

# callbacks botones control manual
def callback_up():
    global s_port
    s_port.write('f'.encode('ascii'))
    print('up')
def callback_down():
    global s_port
    s_port.write('b'.encode('ascii'))
    print('down')
def callback_right():
    global s_port
    s_port.write('d'.encode('ascii'))
    print('rigth')
def callback_left():
    global s_port
    s_port.write('l'.encode('ascii'))
    print('left')

# callback para calibrar
def callback_calibrate():
    global s_port
    s_port.write('c'.encode('ascii'))
    print('calibrate')

# inicio de ventana
root = tk.Tk()
root.title("Brazo Robotico v1.0")
root.geometry(str(root.winfo_screenwidth()-50) + "x" + str(root.winfo_screenheight()-10))

frame1 = tk.Frame(root, width=root.winfo_screenwidth(), 
                    height=root.winfo_screenheight(), bg='#FDFDFF')
frame1.place(anchor='center', relx=0.5, rely=0.5)

# titulo
title_label = tk.Label(frame1, text="Brazo Robotico v1.0",
                        fg="#282c34", bg='#FDFDFF',
                        font=("System",'50', 'bold'),
                        justify='left')
title_label.place(relx=0.5, rely=0.1, anchor='center')

# alpha
alpha_img = ImageTk.PhotoImage(Image.open('imag/transportador.png').resize((128,128)))
alpha_label_img = tk.Label(frame1, image=alpha_img, bg='#FDFDFF')
alpha_label_img.place(relx=0.31, rely=0.325, anchor='e')
alpha_label_text = tk.Label(frame1, text="Alpha",
                            fg="#282c34", bg='#FDFDFF',
                            font=("System",'32', 'bold'),
                            justify='left')
alpha_label_text.place(relx=0.35, rely=0.325, anchor='sw')
alpha_slider = tk.Scale(frame1, from_=0, to=180, orient='horizontal', 
                        length=250, width=25, bg='#FDFDFF')
alpha_slider.place(relx=0.35, rely=0.325, anchor='nw')

# ro
ro_img = ImageTk.PhotoImage(Image.open('imag/scale.png').resize((128,128)))
ro_label_img = tk.Label(frame1, image=ro_img, bg='#FDFDFF')
ro_label_img.place(relx=0.31, rely=0.55, anchor='e')
ro_label_text = tk.Label(frame1, text="Ro",
                        fg="#282c34", bg='#FDFDFF',
                        font=("System",'32', 'bold'),
                        justify='left')
ro_label_text.place(relx=0.35, rely=0.55, anchor='sw')
ro_slider = tk.Scale(frame1, from_=0, to=250, orient='horizontal',
                    length=250, width=25, bg='#FDFDFF')
ro_slider.place(relx=0.35, rely=0.55, anchor='nw')

# boton de go
go = tk.Button(frame1, text='Go', width=10, height=2,
                fg="#FDFDFF", bg='#282c34',
                font=("System",'32', 'bold'),
                justify='center', command=callback_go)
go.place(relx=0.35, rely=0.775, anchor='center')

# control manual
# boton activar control manual
control_manual = False
manual_button = tk.Button(frame1, text="Control Manual",
                            bg="#FDFDFF", fg='#282c34',
                            font=("System",'32', 'bold'),
                            justify='center', command=callback_manual)
manual_button.place(relx=0.66, rely=0.25, anchor='center')
# up button
up_arrow = ImageTk.PhotoImage(Image.open('imag/arrow.png').rotate(90).resize((64,64)))
up_button = tk.Button(frame1, image=up_arrow, 
                        borderwidth=0, bg="#FDFDFF",
                        state='disabled', command=callback_up)
up_button.place(relx=0.66, rely=0.44, anchor='s')
# down button
down_arrow = ImageTk.PhotoImage(Image.open('imag/arrow.png').rotate(-90).resize((64,64)))
down_button = tk.Button(frame1, image=down_arrow, 
                        borderwidth=0, bg="#FDFDFF",
                        state='disabled', command=callback_down)
down_button.place(relx=0.66, rely=0.56, anchor='n')
# right button
right_arrow = ImageTk.PhotoImage(Image.open('imag/arrow.png').resize((64,64)))
right_button = tk.Button(frame1, image=right_arrow, 
                        borderwidth=0, bg="#FDFDFF",
                        state='disabled', command=callback_right)
right_button.place(relx=0.69, rely=0.50, anchor='w')
# left button
left_arrow = ImageTk.PhotoImage(Image.open('imag/arrow.png').rotate(180).resize((64,64)))
left_button = tk.Button(frame1, image=left_arrow, 
                        borderwidth=0, bg="#FDFDFF",
                        state='disabled', command=callback_left)
left_button.place(relx=0.63, rely=0.5, anchor='e')

# calibrate
calibrate_button = tk.Button(frame1, text="Calibrar",
                            width=10, height=2,
                            fg="#FDFDFF", bg='#282c34',
                            font=("System",'24', 'bold'),
                            justify='center', state='disabled',
                            command=callback_calibrate)
calibrate_button.place(relx=0.66, rely=0.775, anchor='center')


root.mainloop()

# cerrar puerto serial
s_port.close()
