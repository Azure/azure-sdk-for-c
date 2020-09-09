// Use container fluid
var containers = $(".container");
containers.removeClass("container");
containers.addClass("container-fluid");

WINDOW_CONTENTS = window.location.href.split('/');
SELECTED_LANGUAGE = 'c';
STORAGE_ACCOUNT_NAME = 'azuresdkdocs';
BLOB_URI_PREFIX = "https://" + STORAGE_ACCOUNT_NAME + ".blob.core.windows.net/$web/" + SELECTED_LANGUAGE + "/";

ATTR1 = '[<span class="hljs-meta">System.ComponentModel.EditorBrowsable</span>]\n<';

// Navbar Hamburger
$(function () {
    $(".navbar-toggle").click(function () {
        $(this).toggleClass("change");
    })
})

// Select list to replace affix on small screens
$(function () {
    var navItems = $(".sideaffix .level1 > li");

    if (navItems.length == 0) {
        return;
    }

    var selector = $("<select/>");
    selector.addClass("form-control visible-sm visible-xs");
    var form = $("<form/>");
    form.append(selector);
    form.prependTo("article");

    selector.change(function () {
        window.location = $(this).find("option:selected").val();
    })

    function work(item, level) {
        var link = item.children('a');

        var text = link.text();

        for (var i = 0; i < level; ++i) {
            text = '&nbsp;&nbsp;' + text;
        }

        selector.append($('<option/>', {
            'value': link.attr('href'),
            'html': text
        }));

        var nested = item.children('ul');

        if (nested.length > 0) {
            nested.children('li').each(function () {
                work($(this), level + 1);
            });
        }
    }

    navItems.each(function () {
        work($(this), 0);
    });
})


function httpGetAsync(targetUrl, callback) {
    var xmlHttp = new XMLHttpRequest();
    xmlHttp.onreadystatechange = function () {
        if (xmlHttp.readyState == 4 && xmlHttp.status == 200)
            callback(xmlHttp.responseText);
    }
    xmlHttp.open("GET", targetUrl, true); // true for asynchronous 
    xmlHttp.send(null);
}

function populateIndexList(selector, packageName) {
    url = BLOB_URI_PREFIX + "docs/versioning/versions"

    httpGetAsync(url, function (responseText) {

        var publishedversions = document.createElement("ul")
        if (responseText) {
            options = responseText.match(/[^\r\n]+/g)

            for (var i in options) {
                $(publishedversions).append('<li><a href="' + getPackageUrl(SELECTED_LANGUAGE, options[i]) + '" target="_blank">' + options[i] + '</a></li>')
            }
        }
        else {
            $(publishedversions).append('<li>No discovered versions present in blob storage.</li>')
        }
        $(selector).after(publishedversions)
    })
}

function getPackageUrl(language, version) {
    return "https://" + STORAGE_ACCOUNT_NAME + ".blob.core.windows.net/$web/" + language + "/docs/" + version + "/index.html";
}

// Populate Versions
$(function () {
    // If this is not the index page (e.g. api/index.html) then it is an api
    // page (e.g. /api/core.html) and we should populate the list of packge
    // versions
    if (WINDOW_CONTENTS[WINDOW_CONTENTS.length - 1] != 'index.html') {
        console.log("Run PopulateList")

        $('article.content > h3').each(function () {
            var pkgName = $(this).text()
            populateIndexList($(this), pkgName)
        })
    }
})