from flask import Flask, request

app = Flask(__name__)

@app.route('/')
def hello_world():
    return 'Hello, World!'

def authenticate_user(username, password):
    # Simulate authentication:
    if username == "user" and password == "password":
        return True
    else:
        return False

@app.route('/authenticate')
def authenticate():
    username = request.args.get('username')
    password = request.args.get('password')

    if authenticate_user(username, password):
        return "Authentication successful!"
    else:
        return "Authentication failed.", 401
    
@app.route('/auth')
def authorize():
    value = request.args.get('value')

    if value == "auth":
        return "Authentication successful!", 200
    else:
        return "Authentication failed.", 401


if __name__ == '__main__':
    app.run(debug=True)