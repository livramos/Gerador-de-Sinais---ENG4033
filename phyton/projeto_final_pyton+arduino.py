import tkinter as tk
from tkinter import ttk, messagebox
from pymongo import MongoClient
import matplotlib.pyplot as plt
from threading import Thread, Timer
import numpy as np
from time import sleep
import json
from serial import Serial


cliente = MongoClient("localhost", 27017)
banco = cliente["gerador_sinais"]
colecao = banco["sinais"]

meu_serial = Serial("COM15", baudrate=9600)


def serial():
  while True:
    if meu_serial != None:
      texto_recebido = meu_serial.readline().decode().strip()
      if texto_recebido != "":
        print(texto_recebido)

        # ESCREVA AQUI O SEU CÓDIGO DA SERIAL!

        

            
    sleep(0.1)
    

# CASO A SERIAL NÃO FUNCIONE, COMENTE A LINHA ABAIXO E DESCOMENTE A SEGUINTE

#meu_serial = None

print("[INFO] Serial: ok")

thread = Thread(target=serial)
thread.daemon = True
thread.start()  

def adicionar_sinal():
    nome = entrada_nome.get()
    tipo = combo_tipo.get()
    formula = entrada_formula.get() #if combo_tipo.get() == "Personalizado" else ""
    periodo = entrada_periodo.get()

    sinal = {
        "nome": nome,
        "tipo": tipo,
        "formula" : formula,
        "periodo" :  periodo
    }
    
    print(sinal)
    colecao.insert_one(sinal)
    atualizar_lista()
    
    
def atualizar_lista():
    #apaga a lista pra nao duplicar
    for item in lista.get_children():
        lista.delete(item)

    #pega os dados do BD, inserindo no formato de arvore com o filho vazio no final
    for sinal in colecao.find():
        lista.insert("", tk.END,
            values=(
                sinal["nome"],
                sinal["tipo"],
                sinal["formula"],
                sinal["periodo"]
            )
        )   

def muda_tipo(event):
    if combo_tipo.get() == "Personalizado":
        label_formula.grid(row=4, column=0)
        entrada_formula.grid(row=4, column=1)
        
        label_periodo.grid(row=5, column=0)
        entrada_periodo.grid(row=5, column=1)
        
    else:
        label_formula.grid_remove()
        entrada_formula.grid_remove()
        
        label_periodo.grid_remove()
        entrada_periodo.grid_remove()


def converter_formula(formula):
    substituicoes = [
        ("sin", "np.sin"),
        ("cos", "np.cos"),
        ("tan", "np.tan"),
        ("asin", "np.arcsin"),
        ("acos", "np.arccos"),
        ("atan", "np.arctan"),
        ("exp", "np.exp"),
        ("log", "np.log"),
        ("pi", "np.pi"),
    ]
    for nome, numpy in substituicoes:
        formula = formula.replace(nome, numpy)
    return formula


def plotar_grafico():
    formula = entrada_formula.get()
    tipo = combo_tipo.get()
    if tipo == "Quadrada":
        x = np.linspace(0,10,1000)
        y = np.sign(np.sin(2 * np.pi * 5 * x))
        plt.plot(x, y)
        plt.show()
    elif tipo == "Senoidal":
        x = np.linspace(0,10,1000)
        y = np.sin(2 * np.pi * 5 * x)
        plt.plot(x, y)
        plt.show()
    elif tipo == "Triangular":
        x = np.linspace(0,10,1000)
        y = (2 / np.pi) * np.arcsin(np.sin(2 * np.pi * 5 * x))
        plt.plot(x, y)
        plt.show()
    else:
        x = np.linspace(0, 10, 1000)
        formula_convertida = converter_formula(formula)
        y = eval(formula_convertida)
        plt.plot(x, y)
        plt.show()

def selecionar_sinal(evento):
    global nome_selecionado
    
    #quando a lista esviver vazia, pra nao dar erro
    selecionado = lista.selection()
    
    if len(selecionado) == 0:
        return
    
    item = lista.selection()[0]
    valores = lista.item(item, "values")
    
    nome_selecionado = valores[0]
    tipo_selecionado = valores[1]
    formula_selecionada = valores[2]
    periodo_selecionado = valores[3]
    
    #apaga o conteudo atual da caixa
    entrada_nome.delete(0, tk.END)
    entrada_nome.insert(0, nome_selecionado)
    
    combo_tipo.set(tipo_selecionado)
    
    if tipo_selecionado != "":
        muda_tipo("<<ComboboxSelected>>")
    
    entrada_formula.delete(0, tk.END)
    entrada_formula.insert(0, formula_selecionada)
    
    entrada_periodo.delete(0, tk.END)
    entrada_periodo.insert(0, periodo_selecionado)
    
    print(valores)
    print(nome_selecionado)
    print(tipo_selecionado)
    print(formula_selecionada)
    print(periodo_selecionado)
    
    
    


def editar_sinal():
    global nome_selecionado
    
    novo_nome = entrada_nome.get()
    novo_tipo = combo_tipo.get()
    nova_formula = entrada_formula.get()
    novo_periodo = entrada_periodo.get()
    
    colecao.update_one(
        {"nome": nome_selecionado},
        {"$set": {
            "nome": novo_nome,
            "tipo": novo_tipo,
            "formula": nova_formula,
            "periodo": novo_periodo
        }}
    )

    nome_selecionado = None
    atualizar_lista()
    


def remover_sinal():
    nome = entrada_nome.get()
    colecao.delete_one({"nome": nome})
    atualizar_lista()


def gerar_pontos():
    tipo = combo_tipo.get()
    periodo = entrada_periodo.get()
    
    if tipo == "Personalizado":
        if periodo == "":
            periodo = "1"
        x = np.linspace(0, float(periodo), 100)
        
    else:
        x = np.linspace(0, 1, 100)
    
    if tipo == "Quadrada":
        y = np.sign(np.sin(2 * np.pi * 1 * x))
    elif tipo == "Senoidal":
        y = np.sin(2 * np.pi * 1 * x)
    elif tipo == "Triangular":
        y = (2 / np.pi) * np.arcsin(np.sin(2 * np.pi * 1 * x))
    else:
        formula = entrada_formula.get()
        formula_convertida = converter_formula(formula)
        y = eval(formula_convertida)
    return x, y

def enviar_arduino():
    x, y = gerar_pontos()
    nome = entrada_nome.get()
    tipo = combo_tipo.get()
    periodo = entrada_periodo.get()
    valores = [round(v, 2) for v in y.tolist()]
    texto_valores = ",".join(str(v) for v in valores)
    texto = "sinal " + nome + ";" + periodo + ";" + texto_valores
    meu_serial.write((texto + "\n").encode())
    print(texto)
    
janela = tk.Tk()
janela.geometry("1250x850")
janela.title("Cadastro de Sinais")

frame = tk.Frame(janela) #meio que um container, so da pra usar o grid e o pack por causa dele

#tk.Label(janela, text="Gerador de Sinais", font=("Arial", 16)).pack(pady=10)
frame.pack(pady=10)
#empilha widgets um abaixo do outro (ou lado a lado)


# nome e criacao da caixa, isso seria uma label vindo de um frame
# frame pq sem ele basicamente colocamos tudo em uma janela
# ficaria mais "bagunçado"
# e com o frame da pra usar o grid, pra deixar tudo alinhado

lista = ttk.Treeview(
    janela,
    columns=("Nome", "Tipo", "Formula", "Periodo"),
    show="headings"
)

for coluna in ("Nome", "Tipo", "Formula", "Periodo"):
    lista.heading(coluna, text=coluna)

# tipo margin-top e margin-bottom do CSS
lista.pack(fill="both", expand=True, padx=10, pady=10)


tk.Label(frame, text="Nome").grid(row=0, column=0)
entrada_nome = tk.Entry(frame)
entrada_nome.grid(row=0, column=1)
# grid como linhas e colunas como uma tabela


tk.Label(frame, text="Tipo").grid(row=1, column=0)
combo_tipo = ttk.Combobox(frame, values=["Quadrada", "Senoidal", "Triangular", "Personalizado"])
combo_tipo.current(0)
combo_tipo.grid(row=1, column=1)
combo_tipo.bind("<<ComboboxSelected>>", muda_tipo)


label_formula = tk.Label(frame, text="Fórmula/valores")
entrada_formula = tk.Entry(frame, width=40)
#entrada_formula.grid(row=4, column=1)

label_periodo = tk.Label(frame, text="Limite do período")
entrada_periodo = tk.Entry(frame, width=10)



tk.Button(janela, text="Adicionar sinal", command=adicionar_sinal).pack(side="left", pady=5)
tk.Button(janela, text="Plotar gráfico", command=plotar_grafico).pack(side="left",  pady=5)
tk.Button(janela, text="Remover Sinal", command=remover_sinal).pack(side="left", pady=5)
tk.Button(janela, text="Editar Sinal", command=editar_sinal).pack(side="left", pady=5)
tk.Button(janela,text="Testar pontos",command=enviar_arduino).pack(side="left", pady=5)

#evento de selecao na tabela <<TreeviewSelect>>
lista.bind("<<TreeviewSelect>>", selecionar_sinal)

atualizar_lista()
janela.mainloop()

