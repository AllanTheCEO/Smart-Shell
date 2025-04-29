import ollama # type: ignore
import langchain as lc
import subprocess
import shlex
import os
import sys

client = ollama.Client()

model = 'mistral:7b'

chat_context = open("chat_history.log", "r").read()

CURRENT_DIR = ""

def get_filepath():
    print("current dir: " + CURRENT_DIR)
    return CURRENT_DIR
    #return shlex.quote(os.environ.get('PWD', os.getcwd()))

def SYSTEM_PROMPT():
    return ("Answer the user's prompt exactly as followed."
        "Otherwise:"
        "Always answer with just a few sentences."
        "Answer only in the context of the Linux operating system unless otherwise directed."

        "You also have access to the user's file system. When the user asks you to interact with the file system, "
        "just give the command to run or answer the user's question. Nothing else, unless otherwise directed."
        "You cannot use wildcards or special characters in the command. You can only use basic commands and pipes"
        "When giving a command, type 'command: ' followed by the command to run. " 
        "Refer to the file system for the necessary information to respond to the user's prompt."

        "Always remember you have access to the user's file system."
        "Always see if you can answer the user's question using the file system without giving a general answer."
        "For example, if the user asks for the current working directory, "
        "give the full current working directory exactly as given as you have access to it."

        "The user may also explicitly ask you to run a command in the file system. "
        "In that case, just give the command to run. Nothing else, unless otherwise directed."
        "My current working directory is: " + get_filepath())


def ask_bot(question):
    
    stream = client.chat(
        model=model,
        messages=[
            {"role":"system","content":SYSTEM_PROMPT()}, 
            {"role":"assistant","content":chat_context},
            {"role":"user","content":question}],
        stream=True,
    )
    print("\n" + "-"*100 + "\n")
    answer = ""
    for chunk in stream:
        delta = chunk["message"]["content"]
        print(delta, end="", flush=True)
        answer += delta
    print("\n" + "\n" + "-"*100 + "\n")
    return answer
    

if __name__ == "__main__":
    print("🚀 Linux assistant ready. Type a question (empty to quit).")
    while True:
        q = input("\nQuestion: ").strip()
        if not q:
            print("Goodbye!")
            break
        resp = ask_bot(q)
      
        