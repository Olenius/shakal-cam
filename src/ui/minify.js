const fs = require("fs");
const { minify } = require("html-minifier-terser");

async function minifyHtml() {
  try {
    const html = fs.readFileSync("index.html", "utf8");
    const minified = await minify(html, {
      collapseWhitespace: true,
      removeComments: true,
      minifyCSS: true,
      minifyJS: true,
      removeRedundantAttributes: true,
      removeEmptyAttributes: true,
      removeOptionalTags: true,
    });

    fs.writeFileSync("../../assets/web/index.min.html", minified);
    console.log("HTML minified successfully!");
  } catch (err) {
    console.error("Error minifying HTML:", err);
  }
}

minifyHtml();
