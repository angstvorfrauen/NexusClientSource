  const shots = document.querySelectorAll('.shot-image');
  const lightbox = document.getElementById('lightbox');
  const lightboxImg = document.getElementById('lightboxImg');
  const tag = document.getElementById('lightboxTag');
  let current = 0;

  const shotData = Array.from(shots).map(s => ({
    src: s.querySelector('img').src,
    alt: s.querySelector('img').alt,
    tag: s.querySelector('.shot-tag').textContent
  }));

  function openLightbox(index) {
    current = index;
    render();
    lightbox.classList.add('is-open');
    document.body.style.overflow = 'hidden';
  }

  function closeLightbox() {
    lightbox.classList.remove('is-open');
    document.body.style.overflow = '';
  }

  function render() {
    lightboxImg.src = shotData[current].src;
    lightboxImg.alt = shotData[current].alt;
    tag.textContent = shotData[current].tag;
  }

  function next() {
    current = (current + 1) % shotData.length;
    render();
  }

  function prev() {
    current = (current - 1 + shotData.length) % shotData.length;
    render();
  }

  shots.forEach(shot => {
    shot.addEventListener('click', () => openLightbox(Number(shot.dataset.index)));
    shot.addEventListener('keydown', e => {
      if (e.key === 'Enter' || e.key === ' ') {
        e.preventDefault();
        openLightbox(Number(shot.dataset.index));
      }
    });
  });

  document.getElementById('lightboxClose').addEventListener('click', closeLightbox);
  document.getElementById('lightboxNext').addEventListener('click', next);
  document.getElementById('lightboxPrev').addEventListener('click', prev);

  lightbox.addEventListener('click', e => {
    if (e.target === lightbox) closeLightbox();
  });

  document.addEventListener('keydown', e => {
    if (!lightbox.classList.contains('is-open')) return;
    if (e.key === 'Escape') closeLightbox();
    if (e.key === 'ArrowRight') next();
    if (e.key === 'ArrowLeft') prev();
  });