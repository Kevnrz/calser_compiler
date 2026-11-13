// Prueba de la lógica SVG con elementos simulados; no sustituye revisión visual.
const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const html=fs.readFileSync(require('node:path').join(__dirname,'../../tools/visor_ast.html'),'utf8');
const sample=html.match(/<script id="sample" type="application\/json">([\s\S]*?)<\/script>/)[1];
const script=html.match(/<script>\s*([\s\S]*?)<\/script>/)[1];
class Element{
 constructor(){this.children=[];this.attrs={};this.textContent='';this.style={setProperty(){}};this.classList={remove(){},add(){}};this.clientWidth=1100;this.clientHeight=700;}
 append(e){this.children.push(e);}setAttribute(k,v){this.attrs[k]=String(v);}replaceChildren(...elements){this.children=elements;}addEventListener(){}
}
const elements=new Map();const document={getElementById(id){if(!elements.has(id))elements.set(id,new Element());return elements.get(id);},createElement(){return new Element();},createElementNS(){return new Element();}};
document.getElementById('sample').textContent=sample;
const context=vm.createContext({document,assert,console});
vm.runInContext(script+`
assert(visible.length<all.length);
collapsed.clear();draw();
assert.equal(visible.length,all.length);
assert.equal($('nodes').children.length,all.length);
assert.equal($('edges').children.length,all.length-1);
for(const n of visible){const p=positions.get(n.id);assert(Number.isFinite(p.x)&&Number.isFinite(p.y));for(const c of n.children)assert(positions.get(c.id).y>p.y);}
const levels=new Map();for(const p of positions.values()){if(!levels.has(p.y))levels.set(p.y,[]);levels.get(p.y).push(p.x);}
for(const xs of levels.values()){xs.sort((a,b)=>a-b);for(let i=1;i<xs.length;i++)assert(xs[i]-xs[i-1]>=W);}
// Ninguna caja debe solaparse con otra en la distribución clásica.
const points=[...positions.values()];for(let i=0;i<points.length;i++)for(let j=i+1;j<points.length;j++)assert(Math.abs(points[i].x-points[j].x)>=W||Math.abs(points[i].y-points[j].y)>=H);
// Los hermanos vuelven a compartir fila; las ramas tienen espacio vertical.
for(const n of visible)for(const c of n.children)assert.equal(positions.get(c.id).y-positions.get(n.id).y,LEVEL);
assert(LEVEL-H>=120);
assert(W<186);
const branch=all.find(n=>n.kind==='CRYPTO');select(branch);assert.equal(selected,branch);
const before=visible.length;fold(branch);assert(visible.length<before);assert(!visible.includes(branch.children[0]));fold(branch);assert.equal(visible.length,before);
fit();assert(scale>0&&Number.isFinite(tx)&&Number.isFinite(ty));
const x=100,y=200,worldX=(x-tx)/scale,worldY=(y-ty)/scale;zoom(1.25,x,y);assert(Math.abs((x-tx)/scale-worldX)<1e-8);assert(Math.abs((y-ty)/scale-worldY)<1e-8);
const bad=JSON.parse($('sample').textContent);bad.root.children[0].id=bad.root.id;assert.throws(()=>validate(bad));
assert.throws(()=>validate({schema:'otro',version:1,root:{}}));
console.log('PASS visor: ramas completas, niveles sin solapamiento, selección, plegado, zoom y validación JSON');
`,context);
