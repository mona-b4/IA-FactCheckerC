

import subprocess
import json

def fact_check(question):
    try:
        with open("question.txt", "w", encoding="utf-8") as f:
            f.write(question)

        # Appelle le programme C compilé
        subprocess.run(["../agent.exe"], check=True)

        with open("ai_result.json", "r", encoding="utf-8") as f:
            result = json.load(f)

        return result
    except Exception as e:
         return {
    "answer": result.get("answer", ""),
    "confidence": result.get("confidence", 0),
    "explanation": result.get("explanation", ""),
    "sources": result.get("sources", [])
        }

      

