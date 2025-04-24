import ollama
import os
from langchain_ollama.llms import OllamaLLM
from langchain.document_loaders import DirectoryLoader, TextLoader
from langchain.text_splitter import CharacterTextSplitter
from langchain.embeddings import HuggingFaceEmbeddings
from langchain.vectorstores import FAISS
from langchain import LLMChain
from langchain_core.prompts import ChatPromptTemplate


model = OllamaLLM(model='shortbot:latest')

filepath = 'linux-commands/man_text/linuxcommand.org/lc3_man_pages'

loader = DirectoryLoader(
    filepath,
    glob="**/*.txt",
    loader_cls=TextLoader,
    loader_kwargs={"encoding": "utf-8"},
)

docs = loader.load()

splitter = CharacterTextSplitter(chunk_size=800, chunk_overlap=100)
chunks = splitter.split_documents(docs)

embeddings = HuggingFaceEmbeddings(model_name="all-MiniLM-L6-v2")

if os.path.isdir("faiss_man_pages_index"):
    faiss_index = FAISS.load_local("faiss_man_pages_index", embeddings, allow_dangerous_deserialization=True)
else:
    faiss_index = FAISS.from_documents(chunks, embeddings)
    faiss_index.save_local("faiss_man_pages_index")


model = OllamaLLM(model='shortbot:latest')


def answer(query: str) -> str:
    '''
    results = faiss_index.similarity_search(query, k=5)
    context = "\n\n".join(doc.page_content for doc in results)
    '''
    template = f"""
    You are a Linux terminal assistant. Use the following man-page excerpts to answer the question.

    CONTEXT:
    {context}

    QUESTION:
    {input}

    ANSWER:
    """
    prompt = ChatPromptTemplate.from_template(template)

    chain = prompt | model
    answer = chain.invoke({"context": context, "input": query})
    return answer

if __name__ == "__main__":
    print("🚀 Man-page assistant ready. Type a question (empty to quit).")
    while True:
        q = input("\nQuestion: ").strip()
        if not q:
            print("Goodbye!")
            break
        resp = answer(q)
        print("\n" + "-"*40)
        print(resp)
