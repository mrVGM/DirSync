let _panel;

const { render } = require('./renderHTML');

function getPanel() {
    if (!_panel) {
        _panel = render('modal');
        document.getElementById('main').appendChild(_panel.element);
    }
    
    return _panel;
}

function setVisible(visible) {
    const panel = getPanel();
    panel.element.style.display = visible ? 'block' : 'none';
}

async function choose(options, title) {
    const panel = getPanel();

    setVisible(true);

    panel.tagged.options_space.innerHTML = '';
    if (title) {
        panel.tagged.header.innerHTML = title;
    }

    const chosen = await new Promise((resolve, reject) => {
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

    setVisible(false);

    return chosen;
}

exports.choose = choose;