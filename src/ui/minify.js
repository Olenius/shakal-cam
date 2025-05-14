const fs = require("fs");
const { minify } = require("html-minifier-terser");

async function minifyHtml(filename) {
  try {
    const inputFile = `${filename}.html`;
    const outputFile = `../../assets/web/${filename}.min.html`;
    
    const html = fs.readFileSync(inputFile, "utf8");
    const minified = await minify(html, {
      collapseWhitespace: true,
      removeComments: true,
      minifyCSS: true,
      minifyJS: true,
      removeRedundantAttributes: true,
      removeEmptyAttributes: true,
      removeOptionalTags: true,
    });

    fs.writeFileSync(outputFile, minified);
    console.log(`${inputFile} minified successfully!`);
  } catch (err) {
    console.error(`Error minifying ${filename}.html:`, err);
  }
}

async function processAllFiles() {
  const files = ["index", "gallery", "settings"];
  
  for (const file of files) {
    await minifyHtml(file);
  }
  
  console.log("All HTML files have been minified!");
}

processAllFiles();
