import ollama # type: ignore

client = ollama.Client()

model = 'shortbot:latest'

chat_context = open("chat_history.log", "r").read()
    

SYSTEM_PROMPT = (
    "Answer the user's prompt exactly as followed."
    "Otherwise:"
    "Always answer with just a few sentences."
    "Answer only in the context of the Linux operating system unless otherwise directed."
)

def ask_bot(question):
    stream = client.chat(
        model=model,
        messages=[
            {"role":"system","content":SYSTEM_PROMPT},
            {"role":"system","content":chat_context},
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
        
        