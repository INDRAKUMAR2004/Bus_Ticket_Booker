// API URL Configuration
// When deploying to Netlify, this ensures the frontend talks to your Render server.
// Replace the Render URL with your actual deployed Render server URL.
const IS_LOCAL = window.location.hostname === 'localhost' || window.location.hostname === '127.0.0.1' || window.location.protocol === 'file:';
const API_BASE = IS_LOCAL ? 'http://localhost:8080' : 'https://bus-ticket-booker.onrender.com'; // ⬅️ Replace this with your actual Render URL!

document.addEventListener('DOMContentLoaded', () => {
    // --- Booking Form Logic ---
    const bookingForm = document.getElementById('booking-form');
    if (bookingForm) {
        // Element References
        const els = ['name', 'phone', 'from', 'to', 'date', 'seats'].reduce((acc, id) => {
            acc[id] = document.getElementById(id);
            return acc;
        }, {});

        const prevs = ['name', 'from', 'to', 'date', 'seats'].reduce((acc, id) => {
            acc[id] = document.getElementById(`prev-${id}`);
            return acc;
        }, {});

        const submitBtn = document.getElementById('submitBtn');
        const loader = document.getElementById('loader');
        const statusMsg = document.getElementById('status-message');
        
        // 1. Auto-fill from LocalStorage
        const savedData = JSON.parse(localStorage.getItem('luxuryBusData') || '{}');
        if(savedData.name) { els.name.value = savedData.name; els.name.classList.add('has-value'); prevs.name.textContent = savedData.name;}
        if(savedData.phone) { els.phone.value = savedData.phone; els.phone.classList.add('has-value'); }

        // Set min date to today
        const today = new Date().toISOString().split('T')[0];
        if(els.date) els.date.setAttribute('min', today);

        // 2. Real-time Preview Update
        Object.keys(els).forEach(key => {
            if(!els[key]) return;
            els[key].addEventListener('input', (e) => {
                const val = e.target.value;
                if(val) e.target.classList.add('has-value');
                else e.target.classList.remove('has-value');

                if (prevs[key]) {
                    prevs[key].textContent = val || (key === 'seats' ? '1' : '--');
                }
            });
        });

        // 3. Swap functionality
        const swapIcon = document.querySelector('.icon-swap');
        if (swapIcon) {
            swapIcon.addEventListener('click', () => {
                const temp = els.from.value;
                els.from.value = els.to.value;
                els.to.value = temp;
                
                // update classes
                if(els.from.value) els.from.classList.add('has-value'); else els.from.classList.remove('has-value');
                if(els.to.value) els.to.classList.add('has-value'); else els.to.classList.remove('has-value');

                // update prevs
                prevs.from.textContent = els.from.value || '--';
                prevs.to.textContent = els.to.value || '--';
            });
            swapIcon.style.cursor = 'pointer';
        }

        // 4. Form Submission
        bookingForm.addEventListener('submit', async (e) => {
            e.preventDefault();
            
            // Basic validation
            if (els.phone.value.length < 10) {
                showStatus('Phone number must be valid.', 'error');
                return;
            }

            // Save to localStorage
            localStorage.setItem('luxuryBusData', JSON.stringify({
                name: els.name.value,
                phone: els.phone.value
            }));

            // UI State change
            submitBtn.classList.add('hidden');
            loader.classList.remove('hidden');
            statusMsg.textContent = '';
            
            // Prepare Data for JSON payload
            const payload = {
                name: els.name.value,
                phone: els.phone.value,
                from: els.from.value,
                to: els.to.value,
                date: els.date.value,
                seats: els.seats.value
            };

            try {
                // Fetch to POST JSON data
                const response = await fetch(`${API_BASE}/book`, {
                    method: 'POST',
                    headers: { 
                        'Content-Type': 'application/json' 
                    },
                    body: JSON.stringify(payload)
                });

                const data = await response.json();

                if (response.ok && data.bookingId) {
                    // Success UI
                    await fetchTicketDetails(data.ticketUrl);
                } else {
                    throw new Error(data.message || 'Booking failed');
                }
            } catch (error) {
                showStatus(error.message || 'Network Error. Is the backend running?', 'error');
                submitBtn.classList.remove('hidden');
            } finally {
                loader.classList.add('hidden');
            }
        });

        // 5. Fetch Ticket Details & Redirect
        async function fetchTicketDetails(url) {
            try {
                const response = await fetch(`${API_BASE}${url}`);
                const data = await response.json();
                
                if(response.ok && data.ticket) {
                    sessionStorage.setItem('finalTicket', data.ticket);
                    window.location.href = './success.html'; // Redirect to success page
                } else {
                     throw new Error('Ticket generation failed.');
                }
            } catch (err) {
                 showStatus('Booking worked, but failed to load ticket preview.', 'error');
                 submitBtn.classList.remove('hidden');
            }
        }

        function showStatus(msg, type) {
            statusMsg.textContent = msg;
            statusMsg.className = `status-message mt-2 ${type}`;
            statusMsg.style.display = 'block';
        }
    }
});
