import ollama # type: ignore

client = ollama.Client()

model = 'shortbot:latest'

SYSTEM_PROMPT = "Always answer with just a few sentences unless otherwise directed. " \
"Answer only in the context of the Linux operating system unless otherwise directed."


def ask_bot(question):
    response = client.generate(model=model, prompt=question + SYSTEM_PROMPT)
    return response.response
    

if __name__ == "__main__":
    print("🚀 Linux assistant ready. Type a question (empty to quit).")
    while True:
        q = input("\nQuestion: ").strip()
        if not q:
            print("Goodbye!")
            break
        resp = ask_bot(q)
        print("\n" + "-"*100 + "\n")
        print(resp, end="", flush=True)
        
        