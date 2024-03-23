let _panel;
let _cancelButton;
let _handler;

const { render } = require('./renderHTML');

function getPanel() {
    if (!_panel) {
        _panel = render('modal');

        const button = render('button');
        _cancelButton = button;
        _panel.tagged.cancel_button.appendChild(button.element);

        button.tagged.name.innerHTML = 'Cancel';
        button.element.addEventListener('click', () => {
            if (_handler) {
                _handler();
            }
            _handler = undefined;
        });
    }
    
    return _panel;
}

async function setVisible(visible) {
    const panel = getPanel();
    let mainMod = await app.modules.main;

    if (visible) {
        mainMod.tagged.bar_space.appendChild(panel.element);
    }
    else {
        panel.element.remove();
    }
}

async function choose(options, title) {
    const panel = getPanel();

    await setVisible(true);

    panel.tagged.options_space.innerHTML = '';
    if (title) {
        panel.tagged.header.innerHTML = title;
    }

    const chosen = await new Promise((resolve, reject) => {
        _handler = () => {
            resolve();
        };

        for (let i = 0; i < options.length; ++i) {
            const cur = options[i];
            const item = render('option');
            item.tagged.name.innerHTML = cur.name;
            item.tagged.option.addEventListener('click', () => {
                resolve(cur);
            });
            panel.tagged.options_space.appendChild(item.element);
        }
    });

    await setVisible(false);

    return chosen;
}

exports.choose = choose;