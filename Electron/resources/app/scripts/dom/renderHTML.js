function render(name) {
    const src = app.html[name];

    let tmp = document.createElement('template');
    tmp.innerHTML = src.contents;

    let root;
    for (let i = 0; i < tmp.content.childNodes.length; ++i) {
        const cur = tmp.content.childNodes[i];
        if (cur.nodeName !== '#text') {
            root = cur;
            break;
        }
    }
    
    function* getAllChildren(node) {
        yield node;
        for (let i = 0; i < node.childNodes.length; ++i) {
            const cur = node.childNodes[i];
            const it = getAllChildren(cur);

            let tmp = it.next();
            while (!tmp.done) {
                yield tmp.value;
                tmp = it.next();
            }
        }
    }

    const res = {
        element: root,
        tagged: {}
    }

    const it = getAllChildren(root);
    let cur = it.next();

    while (!cur.done) {
        const elem = cur.value;
        cur = it.next();

        let tag;
        if (elem.getAttribute) {
            tag = elem.getAttribute('tag');
        }
        if (tag) {
            res.tagged[tag] = elem;
        }
    }
    
    return res;
}

exports.render = render;