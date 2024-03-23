let _panel;

const { render } = require('./renderHTML');

function init() {
    const panel = getPanel();

    panel.interface = {
        show: message => {
            const frame = render('notification_frame');
            
            frame.tagged.text.innerHTML = message;

            panel.tagged.root.appendChild(frame.element);

            setTimeout(() => {
                frame.element.style.opacity = 0;

                setTimeout(() => {
                    frame.element.remove();
                }, 2000);
            }, 2000);
        }
    };

    return panel;
}

function getPanel() {
    if (!_panel) {
        _panel = render('notification');
        document.getElementById('main').appendChild(_panel.element);
    }
    
    return _panel;
}


exports.init = init;