from flask import Flask, render_template, request, redirect, url_for

app = Flask(__name__)


@app.route('/')
def index():
    return render_template('index.html')


@app.route('/user', methods=['GET', 'POST'])
def user():
    if request.method == 'POST':
        try:
            # Read weight and check if it is valid
            weight = request.form['weight']
            weight = round(float(weight), 1)
            if weight > 0 and weight <= 1000:
                f = open('data/user.txt', 'w')
                f.write("weight:" + str(weight))
                f.close()
                return redirect(url_for('index'))
            else:
                raise ValueError
        except (ValueError, OSError):
            return render_template('user_failed.html')
    elif request.method == 'GET':
        return render_template('user.html')


@app.route('/session/')
def session():
    try:
        # Read session data
        f = open("data/session.txt","r")
        lines = f.readlines()
        data = dict()
        for l in lines:
            l = l.strip().split(":")
            data[l[0]] = int(l[1])
        # Read user data
        f = open("data/user.txt","r")
        lines = f.readlines()
        for l in lines:
            l = l.strip().split(":")
            data[l[0]] = float(l[1])
        cals = int(calculate_calories(data["weight"], data["dist"]))
        return render_template('session.html', steps=int(data["steps"]), dist=data["dist"], cals=cals)
    except (ValueError, OSError):
        return render_template('session_failed.html')

def calculate_calories(weight, dist):
    met = 4.8
    speed = 5.0
    time = 3600*(dist/1000)/speed
    calories = (time*met*3.5*weight)/(200*60)
    return calories


if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0')