from flask import Flask, render_template, request, jsonify
from fact_check import fact_check

app = Flask(__name__)

@app.route("/")
def index():
    return render_template("index.html")

@app.route("/api/ask", methods=["POST"])
def ask():
    data = request.get_json()
    question = data.get("question", "")
    result = fact_check(question)
    return jsonify(result)

if __name__ == "__main__":
    app.run(debug=True)
